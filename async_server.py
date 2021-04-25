import asyncio
import zmq
import zmq.asyncio
import json

#if length >>>
#see onlines

ctx = zmq.asyncio.Context()

id_to_name = {}

def name_to_id(name):
    for i in id_to_name.keys():
        if id_to_name[i] == name:
            return i
    return None

def handle_send(params):
    receiver_name = params['to']
    receiver_id = name_to_id(receiver_name)
    
    processed_msg = {}
    processed_msg['params'] = params
    processed_msg['method'] = 'receive_message'

    return json.dumps(processed_msg),receiver_id


async def async_process(message_queue):

    message = await message_queue.get()
    # print(message)
    message_ = (message.strip('][').split(', ')[2])[2:-1]
    user_id = (message.strip('][').split(', ')[0])[2:-1]
    json_message = json.loads(message_)

    
    if json_message['method'] == 'send_message':
        params = json_message['params']
        ready_to_be_sent , receiver_id= handle_send(params)
        return ready_to_be_sent, receiver_id

    elif json_message['method'] == 'login':

        name = json_message['params']['name']
        global id_to_name
        id_to_name[user_id] = name

        return name, None

async def main():
    
    sock = ctx.socket(zmq.ROUTER)
    sock.bind('tcp://*:5672')
    print('Connected')

    message_queue = asyncio.Queue()

    while True:
        received_msg = await sock.recv_multipart()
        message_queue.put_nowait(received_msg.__str__())
        

        message, be_send = await async_process(message_queue)
        if be_send:
            
            await sock.send_multipart([be_send.encode(),b"",message.encode()])
        else:
            
            print(f'{message} connected.')


if __name__ == '__main__':
    asyncio.run(main())
