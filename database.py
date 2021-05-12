from sys import stderr
from utils import quote, bcolors
from cassandra.cluster import Cluter()
import object

class DataBase:
    def __init__(
            self,
            dbName='',
            user='',
            password='',
            host='localhost',
            port=5432,
            schemas={}):

        self.dbName = dbName
        self.user = user
        self.password = password
        self.host = host
        self.port = port
        self.schemas = schemas

        self.conn = None
        self.cursor = None

    def exectue_query(self, query):

        result = None
        query = query.replace('\"', '\'').replace('\'NULL_VALUE\'', 'NULL')
        try:

            self.cursor.execute(query)

            try:
                result = self.cursor.fetchall()

            except Exception as err:
                pass

        except Exception as err:
            err = f'{bcolors.FAIL}Error from PostgreSQL: ' + str(err)
            stderr.write(err)
            self.conn.rollback()
            self.conn.close()
            self.connect()
            return 1, err

        else:
            print(f'{bcolors.OKBLUE}log: ', f'{bcolors.OKGREEN}{self.cursor.statusmessage}')
            self.conn.commit()
            self.conn.close()
            self.connect()
            return 0, result

    def connect(self):
        self.conn = psycopg2.connect(
            database=self.dbName,
            user=self.user,
            password=self.password,
            host=self.host,
            port=self.port)
        self.cursor = self.conn.cursor()

    def insert(self, obj):

        attributes = ''
        values = ''

        items = obj.__dict__.items()
        class_name = obj.__class__.__name__

        is_first = True  # for first ','
        for i in items:

            if is_first:
                attributes = attributes + i[0]

        # If is None, then insert NULL
                if i[1] is None:
                    i[1] = 'NULL'
                values = values + quote(str(i[1]))
                is_first = False
                continue

            attributes = attributes + ' , ' + i[0]
            val = i[1]
            if i[1] is None:
                i[1] = 'NULL'
            val = quote(str(i[1]))
            values = values + ' , ' + val

        insert_query = f'INSERT INTO {class_name}({attributes}) VALUES({values})'
        err, result = self.exectue_query(insert_query)
        if err:
            return result
        return None

    def delete(self, table, column, key):

        query = f'DELETE FROM {table} WHERE {column} = {quote(key)}'
        err, result = self.exectue_query(query)
        if err:
            return result
        return None

    def update(self, table, u_column, update_to, current_value):
        query = f'UPDATE {table} SET {u_column} = {quote(update_to)} WHERE {u_column} = {quote(current_value)}'

        err, result = self.exectue_query(query)
        if err:
            return result
        return None

    def get(self, table):

        select_query = f'SELECT * FROM {table}'
        err, result = self.exectue_query(select_query)

        return result
