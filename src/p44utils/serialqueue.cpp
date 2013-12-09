//
//  Copyright (c) 2013 plan44.ch / Lukas Zeller, Zurich, Switzerland
//
//  Author: Lukas Zeller <luz@plan44.ch>
//
//  This file is part of p44utils.
//
//  p44utils is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  p44utils is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with p44utils. If not, see <http://www.gnu.org/licenses/>.
//

#include "serialqueue.hpp"

using namespace p44;


#define DEFAULT_RECEIVE_TIMEOUT 3000000 // [uS] = 3 seconds


#pragma mark - SerialOperation


SerialOperation::SerialOperation(SerialOperationFinalizeCB aCallback) :
  callback(aCallback)
{
}

// set transmitter
void SerialOperation::setTransmitter(SerialOperationTransmitter aTransmitter)
{
  transmitter = aTransmitter;
}


// call to deliver received bytes
size_t SerialOperation::acceptBytes(size_t aNumBytes, uint8_t *aBytes)
{
  return 0;
}


OperationPtr SerialOperation::finalize(OperationQueue *aQueueP)
{
  if (callback) {
    callback(*this,aQueueP,ErrorPtr());
    callback = NULL; // call once only
  }
  return OperationPtr(); // no operation to insert
}


void SerialOperation::abortOperation(ErrorPtr aError)
{
  if (callback && !aborted) {
    aborted = true;
    callback(*this,NULL,aError);
    callback = NULL; // call once only
  }
}




#pragma mark - SerialOperationSend


SerialOperationSend::SerialOperationSend(size_t aNumBytes, uint8_t *aBytes, SerialOperationFinalizeCB aCallback) :
  inherited(aCallback),
  dataP(NULL)
{
  // copy data
  setDataSize(aNumBytes);
  appendData(aNumBytes, aBytes);
}


SerialOperationSend::~SerialOperationSend()
{
  clearData();
}




void SerialOperationSend::clearData()
{
  if (dataP) {
    delete [] dataP;
    dataP = NULL;
  }
  dataSize = 0;
  appendIndex = 0;
}


void SerialOperationSend::setDataSize(size_t aDataSize)
{
  clearData();
  if (aDataSize>0) {
    dataSize = aDataSize;
    dataP = new uint8_t[dataSize];
  }
}


void SerialOperationSend::appendData(size_t aNumBytes, uint8_t *aBytes)
{
  if (appendIndex+aNumBytes>dataSize)
    aNumBytes = dataSize-appendIndex;
  if (aNumBytes>0) {
    memcpy(dataP, aBytes, aNumBytes);
  }
}




bool SerialOperationSend::initiate()
{
  if (!canInitiate()) return false;
  size_t res;
  if (dataP && transmitter) {
    // transmit
    res = transmitter(dataSize,dataP);
    if (res!=dataSize) {
      // error
      abortOperation(ErrorPtr(new SQError(SQErrorTransmit)));
    }
    // early release
    clearData();
  }
  // executed
  return inherited::initiate();
}



#pragma mark - SerialOperationReceive


SerialOperationReceive::SerialOperationReceive(size_t aExpectedBytes, SerialOperationFinalizeCB aCallback) :
  inherited(aCallback)
{
  // allocate buffer
  expectedBytes = aExpectedBytes;
  dataP = new uint8_t[expectedBytes];
  dataIndex = 0;
  setTimeout(DEFAULT_RECEIVE_TIMEOUT);
};


SerialOperationReceive::~SerialOperationReceive()
{
  clearData();
}



void SerialOperationReceive::clearData()
{
  if (dataP) {
    delete [] dataP;
    dataP = NULL;
  }
  expectedBytes = 0;
  dataIndex = 0;
}


size_t SerialOperationReceive::acceptBytes(size_t aNumBytes, uint8_t *aBytes)
{
  // append bytes into buffer
  if (!initiated)
    return 0; // cannot accept bytes when not yet initiated
  if (aNumBytes>expectedBytes)
    aNumBytes = expectedBytes;
  if (aNumBytes>0) {
    memcpy(dataP+dataIndex, aBytes, aNumBytes);
    dataIndex += aNumBytes;
    expectedBytes -= aNumBytes;
  }
  // return number of bytes actually accepted
  return aNumBytes;
}


bool SerialOperationReceive::hasCompleted()
{
  // completed if all expected bytes received
  return expectedBytes<=0;
}


void SerialOperationReceive::abortOperation(ErrorPtr aError)
{
  clearData(); // don't expect any more, early release
  inherited::abortOperation(aError);
}


#pragma mark - SerialOperationSendAndReceive


SerialOperationSendAndReceive::SerialOperationSendAndReceive(size_t aNumBytes, uint8_t *aBytes, size_t aExpectedBytes, SerialOperationFinalizeCB aCallback) :
  inherited(aNumBytes, aBytes, aCallback),
  expectedBytes(aExpectedBytes)
{
};


OperationPtr SerialOperationSendAndReceive::finalize(OperationQueue *aQueueP)
{
  if (aQueueP) {
    // insert receive operation
    SerialOperationPtr op(new SerialOperationReceive(expectedBytes, callback)); // inherit completion callback
    callback = NULL; // prevent it to be called from this object!
    return op;
  }
  return inherited::finalize(aQueueP); // default
}


#pragma mark - SerialOperationQueue


// Link into mainloop
SerialOperationQueue::SerialOperationQueue(SyncIOMainLoop &aMainLoop) :
  inherited(aMainLoop),
  fdToMonitor(-1)
{
}


SerialOperationQueue::~SerialOperationQueue()
{
	setFDtoMonitor(-1); // causes unregistering from mainloop
}


// set transmitter
void SerialOperationQueue::setTransmitter(SerialOperationTransmitter aTransmitter)
{
  transmitter = aTransmitter;
}


// set receiver
void SerialOperationQueue::setReceiver(SerialOperationReceiver aReceiver)
{
  receiver = aReceiver;
}


// set filedescriptor to be monitored by SyncIO mainloop
void SerialOperationQueue::setFDtoMonitor(int aFileDescriptor)
{
  if (aFileDescriptor!=fdToMonitor) {
    // unregister previous one, if any
    if (fdToMonitor>=0) {
      SyncIOMainLoop::currentMainLoop().unregisterPollHandler(fdToMonitor);
    }
    // unregister new one, if any
    if (aFileDescriptor>=0) {
      // register
      SyncIOMainLoop::currentMainLoop().registerPollHandler(
        aFileDescriptor,
        POLLIN,
        boost::bind(&SerialOperationQueue::pollHandler, this, _1, _2, _3, _4)
      );
    }
    // save new FD
    fdToMonitor = aFileDescriptor;
  }
}


#define RECBUFFER_SIZE 100

bool SerialOperationQueue::pollHandler(SyncIOMainLoop &aMainLoop, MLMicroSeconds aCycleStartTime, int aFD, int aPollFlags)
{
  if (receiver) {
    uint8_t buffer[RECBUFFER_SIZE];
    size_t numBytes = receiver(RECBUFFER_SIZE, buffer);
    if (numBytes>0) {
      acceptBytes(numBytes, buffer);
    }
  }
  return true;
}


//bool SerialOperationQueue::readyForWrite(SyncIOMainLoop &aMainLoop, MLMicroSeconds aCycleStartTime, int aFD)
//{
//
//  return true;
//}
//
//
//bool SerialOperationQueue::errorOccurred(SyncIOMainLoop &aMainLoop, MLMicroSeconds aCycleStartTime, int aFD)
//{
//
//  return true;
//}




// queue a new serial I/O operation
void SerialOperationQueue::queueSerialOperation(SerialOperationPtr aOperation)
{
  aOperation->setTransmitter(transmitter);
  inherited::queueOperation(aOperation);
}


// deliver bytes to the most recent waiting operation
size_t SerialOperationQueue::acceptBytes(size_t aNumBytes, uint8_t *aBytes)
{
  // first check if some operations still need processing
  processOperations();
  // let operations receive bytes
  size_t acceptedBytes = 0;
  for (OperationList::iterator pos = operationQueue.begin(); pos!=operationQueue.end(); ++pos) {
		SerialOperationPtr sop = boost::dynamic_pointer_cast<SerialOperation>(*pos);
    size_t consumed = 0;
		if (sop)
			consumed = sop->acceptBytes(aNumBytes, aBytes);
    aBytes += consumed; // advance pointer
    aNumBytes -= consumed; // count
    acceptedBytes += consumed;
    if (aNumBytes<=0)
      break; // all bytes consumed
  }
  if (aNumBytes>0) {
    // Still bytes left to accept
    // TODO: possibly create "unexpected receive" operation
  }
  // check if some operations might be complete now
  processOperations();
  // return number of accepted bytes
  return acceptedBytes;
};



