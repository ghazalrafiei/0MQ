import random
from datetime import datetime

import zmq
import zmq.asyncio
import asyncio

import json

from cassandra.cluster import Cluster
from cassandra.cqlengine import columns
from cassandra.cqlengine import connection
from datetime import datetime
from cassandra.cqlengine.management import sync_table
from cassandra.cqlengine.models import Model

import object


id_to_name = {}
online_users_id = set()

def database_connect():
    connection.setup(['127.0.0.1'], 'jikjik_db', protocol_version=3)
    sync_table(object.login_log)
    sync_table(object.message)
    sync_table(object.user)


def name_to_id(name):

    for i in id_to_name.keys():
        if id_to_name[i] == name:
            return i

    return None


def sign_up(params,user_id):

    global id_to_name
    succeed = 'succeed' if params['username'] not in id_to_name.values(
        ) else 'unsuccessful'
    if succeed == 'succeed':
        user_name = params['username']
        id_to_name[user_id] = user_name
        # online_users_id.add(user_id)
        signedup_user = object.user.create(user_name = user_name, user_id = user_id, 
                                            user_signup_timestamp = datetime.now(),
                                            name_pass_hash ='hash:)')
        logged_user = object.login_log.create(user_id=user_id,login_id=random.randint(1000, 2000).__str__(),
                                                    login_timestamp=datetime.now())
    return succeed


def login():
    pass

def create_reply(user_id, params, method='receive_message', result=''):

    processed_msg = {'method': method}

    if method == 'result':
        processed_msg['method'] = method
        processed_msg['params'] = {'result': result}
        receiver_id = user_id

    else:
        receiver_name = params['to']
        receiver_id = name_to_id(receiver_name)
        params['from'] = id_to_name[user_id]
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
        
        sent_status = 'FAILED' if receiver_id is None else 'SUCCEED '
        ready_to_be_sent = None if receiver_id is None else ready_to_be_sent

        message = object.message.create(msg_id = random.randint(3000,4000).__str__(),
                                                msg_content = params['message'],
                                                msg_receiver = params['to'],
                                                msg_sender = params['from'],
                                                msg_status = sent_status,
                                                msg_timestamp = datetime.now())
        return ready_to_be_sent, receiver_id

        
    elif method == 'signup':

        result = sign_up(params,user_id)

        return create_reply(user_id, params, 'result', result)
       


    elif method == 'login':

        # succeed_db = False # see if user and password exists and match
        succeed_db = True if user_name in id_to_name.values(
        ) else False
        succeed = 'succeed' if succeed_db else 'unsuccessful'
        # if succeed_db:
        #     online_users_id.add(user_id)
        if succeed == 'succeed':
                                        
            logged_user = object.login_log.create(user_id=user_id,login_id=random.randint(1000, 2000).__str__(),
                                                  login_timestamp=datetime.now())

        return create_reply(user_id, params, 'result', succeed)


async def main():

    ctx = zmq.asyncio.Context()
    sock = ctx.socket(zmq.ROUTER)
    sock.bind('tcp://{}:{}'.format('*', 5672))
    print('Connected to socket.')

    message_queue = asyncio.Queue()

    while True:
        received_msg = await sock.recv_multipart()
        message_queue.put_nowait(received_msg.__str__())

        message, be_send = await process_request(message_queue)

        if be_send is not None:

            await sock.send_multipart([be_send.encode(), b"", message.encode()])

        elif message is not None:
            
            print(f'{message} connected.')

        elif be_send == None and message == None:
            print("contact does not exists")

if __name__ == '__main__':

    database_connect()
    asyncio.run(main())
