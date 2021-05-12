from cassandra.cluster import Cluster
from datetime import datetime

cluster = Cluster()
session = cluster.connect()
session.set_keyspace('test')
x = session.execute(f'insert into login_log(login_id, login_timestamp, login_user_id) values(0,\'{datetime.now()}\',\'delete\');')
y = session.execute('select * from login_log;')
session.
# print(session.execute("SELECT release_version FROM system.local").one())
# print(x,y)