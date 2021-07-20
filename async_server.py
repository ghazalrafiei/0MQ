import random
from datetime import datetime

import zmq
import zmq.asyncio
import asyncio

import object.object as object
import json
import argparse
import hmac
import hashlib

from dbms import cassandradb, mongodb

KEY = b'9fj3bx8rto8475ljdslkfu8787qa'

_DEBUG_ = False
_DATABASE_ = 'cassandra'
dbms = None


def args_parse():
    
    global _DEBUG_
    debug_parser = argparse.ArgumentParser()
    debug_parser.add_argument('-d', '--debug', help='To log input and output',action='store_true')
    debug_parser.add_argument('-c', '--cassandra', help='To store data in Cassandra DB',action='store_true')
    debug_parser.add_argument('-m', '--mongo', help='To store data in MongoDB',action='store_true')

    args = debug_parser.parse_args()

    if args.debug:
        print('Warning: Debug mode is on.')
        _DEBUG_ = True

    if args.mongo:
        _DATABASE_ = 'mongo'


def user_pass_match(username, password):

    new_hash = hmac.new(KEY, (username+password).encode(),
                        hashlib.sha256).hexdigest()

    stored_hash = ''
    for h in object.user.objects(user_name=username):
        stored_hash = h.name_pass_hash
        break

    return hmac.compare_digest(new_hash, stored_hash)


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
    succeed = 'SUCCEED' if username_count == 0 else 'FAILED'

    if succeed == 'SUCCEED':
        dbms.store_user(username, user_id, datetime.now(),
                        hmac.new(KEY, (username+password).encode(), hashlib.sha256).hexdigest())

        dbms.store_login_log(user_id, random.randint(
            2000, 3000).__str__(), datetime.now())

    return succeed


def login(username, password):

    succeed = 'SUCCEED' if user_pass_match(
        username, password) else 'FAILED'
    user_id = name_to_id(username)

    if succeed == 'SUCCEED':

        dbms.store_login_log(user_id=user_id, login_id=random.randint(2000, 3000).__str__(),
                             login_timestamp=datetime.now())
    return succeed


def create_reply(user_id, params, method='new_message', result = ''):

    processed_msg = {}

    if method == 'result':
        processed_msg['method'] = method
        processed_msg['params'] = {'result': result}
        receiver_id = user_id

    else:
        processed_msg['method'] = method
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

    if method =='new_message':

        ready_to_be_sent, receiver_id = create_reply(user_id, params)
        sent_status = ''

        if receiver_id != user_id:
            sent_status = 'SUCCEED'

        else:  # Sends back the error
            sent_status = 'FAILED'


        dbms.store_message(random.randint(3000, 4000).__str__(),
                           params['message'],
                           params['to'],
                           params['from'],
                           sent_status,
                           datetime.now())

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
        message, receiver = await process_request(message_queue)

        if receiver is not None:

            await sock.send_multipart([receiver.encode(), b"", message.encode()])

            if _DEBUG_ == True:
                print("sent:", [receiver.encode(), b"", message.encode()])

        elif message is not None:

            print(f'{message} connected.')


if __name__ == '__main__':

    args_parse()
    dbms = cassandradb.cassandradb() if _DATABASE_ == 'cassandra' else mongodb.mongodb()
    dbms.connect(['127.0.0.1'], 'jikjik_db')

    asyncio.run(main())
