import asyncio
import zmq
import zmq.asyncio
import json

ctx = zmq.asyncio.Context()

id_to_name = {}
online_users_id = set()

def name_to_id(name):

    for i in id_to_name.keys():
        if id_to_name[i] == name:
            return i
    
    return None

def create_sending_message(user_id, params, method = 'recieve_message', result = ''):

    processed_msg = {}
    processed_msg['method'] = method

    params['from'] = id_to_name[user_id]

    if method == 'result':
        processed_msg['method'] = method
        processed_msg['params'] = {'result' : result}
        receiver_id = params['from']
    
    else:
        receiver_name = params['to']
        receiver_id = name_to_id(receiver_name)
        processed_msg['params'] = params

    return json.dumps(processed_msg),receiver_id

async def process_recieved_message(message_queue):

    message = await message_queue.get()
    message_ = (message.strip('][').split(', ')[2])[2:-1]
    user_id = (message.strip('][').split(', ')[0])[2:-1]
    json_message = json.loads(message_)

    method = json['method']

    if method == 'send_message':

        params = json_message['params']
        #see if online?!
        return create_sending_message(user_id, params)

    elif method == 'signup':

        succeed_db = False # get from db if json_message['params']['username']
        succeed = 'succeed' if succeed_db else 'unsuccessful'
        if succeed_db :
            global id_to_name
            id_to_name[user_id] = json_message['params']['username']
            online_users_id.add(user_id)

        return create_sending_message(user_id, params,'result',succeed)



    elif method == 'login':

        succeed_db = False # see if user and password exists and match
        succeed = 'succeed' if succeed_db else 'unsuccessful'
        if succeed:
            online_users_id.add(user_id)
        
        return create_sending_message(user_id, params,'result',succeed)
        

async def main():
    
    sock = ctx.socket(zmq.ROUTER)
    sock.bind('tcp://{}:{}'.format('*',5672))
    print('Connected')

    message_queue = asyncio.Queue()

    while True:
        received_msg = await sock.recv_multipart()
        message_queue.put_nowait(received_msg.__str__())
        
        message, be_send = await process_recieved_message(message_queue)

        # if message[result] == succedd and was login: print name connected
        await sock.send_multipart([be_send.encode(),b"",message.encode()])



if __name__ == '__main__':
    asyncio.run(main())
