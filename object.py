from datetime import datetime
from cassandra.cqlengine import columns, connection
from cassandra.cqlengine.models import Model

class message(Model):
        
    msg_id = columns.Text(primary_key = True, default = 'Default')
    msg_content = columns.Text()
    msg_receiver = columns.Text()
    msg_sender = columns.Text(index = True)
    msg_status = columns.Text() 
    msg_timestamp = columns.DateTime()

class user(Model):

    user_name = columns.Text(index = True)
    user_id = columns.Text(primary_key = True, default = 'Default')
    user_signup_timestamp = columns.DateTime()
    name_pass_hash = colummns.Text()

class login_log(Model):

    login_id = columns.Integer(primary_key = True)
    user_id = columns.Text()
    login_timestamp = columns.DateTime()
