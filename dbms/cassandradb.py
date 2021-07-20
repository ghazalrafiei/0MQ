import object.object as object

from cassandra.cluster import Cluster
from cassandra.cqlengine import columns
from cassandra.cqlengine import connection
from cassandra.cqlengine.management import sync_table


class cassandradb:
    def connect(self, nodes_address, keyspace):

        connection.setup(nodes_address, keyspace, protocol_version=3)

        sync_table(object.login_log)
        sync_table(object.message)
        sync_table(object.user)

    def store_user(self, username, user_id, now_time, name_pass_hash):
        signed_up_user = object.user.create(user_name=username, user_id=user_id,
                                            user_signup_timestamp=now_time,
                                            name_pass_hash=name_pass_hash)

    def store_message(self, mid, content, to, mfrom, sent_status, now_time):
        message = object.message.create(msg_id=mid,
                                        msg_content=content,
                                        msg_receiver=to,
                                        msg_sender=mfrom,
                                        msg_status=sent_status,
                                        msg_timestamp=now_time)

    def store_login_log(self, user_id, login_id, login_timestamp):
        logged_user = object.login_log.create(
            user_id=user_id, login_id=login_id, login_timestamp=login_timestamp)
