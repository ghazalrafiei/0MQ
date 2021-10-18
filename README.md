# Jikjik
Sending message between two clients using [ZeroMQ](https://zeromq.org/) dealer/router messaging pattern with ```zmq_poll```. Python server and send/receive message in client are asynchronous.

All data such as login and sign up logs, messages and users info are stored in [CassandraDB](https://cassandra.apache.org).

It uses HMAC for authorization.
