import random
from datetime import datetime

import zmq
import zmq.asyncio
import asyncio
from datetime import datetime

import object
import json
import hmac
import hashlib

from cassandra.cluster import Cluster
from cassandra.cqlengine import columns
from cassandra.cqlengine import connection
from cassandra.cqlengine.management import sync_table

KEY = b'9fj3bx8rto8475ljdslkfu8787qa'

_DEBUG_ = True


def database_connect(nodes_address, keyspace):

    connection.setup(nodes_address, keyspace, protocol_version=3)

    sync_table(object.login_log)
    sync_table(object.message)
    sync_table(object.user)

def user_pass_match(username, password):

    new_hash = hmac.new(KEY, (username+password).encode(),hashlib.sha256).hexdigest()
    
    stored_hash = ''
    for h in object.user.objects(user_name = username):
        stored_hash = h.name_pass_hash
        break

    return hmac.compare_digest(new_hash , stored_hash)

def id_to_name(user_id):

    query_result = object.user.objects(user_id=user_id)

    for instance in query_result:
        return instance.user_name

    return None


def name_to_id(username):

    query_result = object.user.objects(user_name=username)

    for instance in query_result:
        return instance.user_id
    
    return None


def sign_up(username, user_id, password):

    query_result = object.user.objects(user_name=username)
    username_count = query_result.count()
    succeed = 'succeed' if username_count == 0 else 'unsuccessful'

    if succeed == 'succeed':
        signed_up_user = object.user.create(user_name=username, user_id=user_id,
                                            user_signup_timestamp=datetime.now(),
                                            name_pass_hash=hmac.new(KEY,(username+password).encode(),hashlib.sha256).hexdigest())
        logged_user = object.login_log.create(user_id=user_id, login_id=random.randint(1000, 2000).__str__(),
                                              login_timestamp=datetime.now())
    return succeed


def login(username, password):

    succeed = 'succeed' if user_pass_match(username, password) else 'unsuccessful'
    user_id = name_to_id(username)

    if succeed == 'succeed':

        logged_user = object.login_log.create(user_id=user_id, login_id=random.randint(1000, 2000).__str__(),
                                              login_timestamp=datetime.now())


def create_reply(user_id, params, method='receive_message', result=''):
    
    processed_msg = {}
    
    if method == 'result':
        processed_msg['method'] = method
        processed_msg['params'] = {'result': result}
        receiver_id = user_id

    else:
        receiver_name = params['to']
        receiver_id = name_to_id(receiver_name)

        if receiver_id is not None:
            params['from'] = id_to_name(user_id)
        
        else:
            params['from'] = 'Server'
            receiver_id = user_id
            processed_msg['method'] = 'error'
            params['emessage'] = 'User doesn\'t exists'
            params['message'] = ''

        processed_msg['params'] = params

    return json.dumps(processed_msg), receiver_id


async def process_request(message_queue):

    message = await message_queue.get()
    message_ = (message.strip('][').split(', ')[2])[2:-1]
    user_id = (message.strip('][').split(', ')[0])[2:-1]

    json_message = json.loads(message_)

    method = json_message['method']
    params = json_message['params']

    if method == 'send_message':

        ready_to_be_sent, receiver_id = create_reply(user_id, params)

        sent_status = ''
        if receiver_id != user_id:
            sent_status = 'SUCCEED '
        
        else: #Sends back the error
            sent_status = 'Failed'
            # ready_to_be_sent = 'User doesn\'t exists'
        

        message = object.message.create(msg_id=random.randint(3000, 4000).__str__(),
                                        msg_content=params['message'],
                                        msg_receiver=params['to'],
                                        msg_sender=params['from'],
                                        msg_status=sent_status,
                                        msg_timestamp=datetime.now())
        # ready_to_be_sent['']

        return ready_to_be_sent, receiver_id

    elif method == 'signup':

        result = sign_up(params['username'], user_id, params['password'])

        return create_reply(user_id, params, 'result', result)

    elif method == 'login':

        result = login(params['username'], params['password'])

        return create_reply(user_id, params, 'result', result)


async def main():

    ctx = zmq.asyncio.Context()
    sock = ctx.socket(zmq.ROUTER)
    
    sock.bind('tcp://{}:{}'.format('*', 5672))

    print('Connected to socket.')

    message_queue = asyncio.Queue()

    while True:
        received_msg = await sock.recv_multipart()

        if _DEBUG_:
            print("received:", received_msg)
        message_queue.put_nowait(received_msg.__str__())

        message, be_sent = await process_request(message_queue)

        if be_sent is not None:
            if _DEBUG_:
                print("sent:",be_sent)
            await sock.send_multipart([be_sent.encode(), b"", message.encode()])

        elif message is not None:
            
            print(f'{message} connected.')


if __name__ == '__main__':

    database_connect(['127.0.0.1'], 'jikjik_db')
    asyncio.run(main())
