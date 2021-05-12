import uuid
from cassandra.cqlengine import columns
from cassandra.cqlengine import connection
from datetime import datetime
from cassandra.cqlengine.management import sync_table
from cassandra.cqlengine.models import Model

class message(Model):

        
    msg_id = columns.Text(primary_key = True, default = 'Default')
    msg_content = columns.Text()
    msg_receiver = columns.Text()
    msg_sender = columns.Text(index = True)
    msg_status = columns.Text()
    msg_timestamp = columns.DateTime()

class login_log(Model):

    log_id = columns.Text(primary_key = True, default = 'Default')
    login_timestamp = columns.DateTime()
    login_user_id = columns.Text()
    name = columns.Text(index = True)
