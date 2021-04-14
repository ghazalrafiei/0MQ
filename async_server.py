import time
import zmq

context = zmq.Context()
socket = context.socket(zmq.ROUTER)
socket.bind('tcp://*:5672')

print('Connected')


while True:
    # identity = socket.recv()
    # print('identity:' , identity)
    print(socket.recv(),socket.recv(),socket.recv())