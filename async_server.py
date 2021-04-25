import asyncio
import zmq
import zmq.asyncio

ctx = zmq.asyncio.Context()

async def async_process(message):
    print(message)
    # process
    return message

async def recv_and_process():
    
    sock = ctx.socket(zmq.ROUTER)
    sock.bind('tcp://*:5672')
    print('Connected')

    while True:
        msg = await sock.recv_multipart()
        print(msg)
        reply = await async_process(msg)
        await sock.send_multipart(reply)


if __name__ == '__main__':
    asyncio.run(recv_and_process())
