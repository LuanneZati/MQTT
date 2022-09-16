from decimal import Decimal
import json
from unicodedata import decimal
from main import TOPIC, CLIENT_ID, mqtt_connection
from awscrt import mqtt
import time
import json
# Bibliotecas MySql
import mysql.connector
from mysql.connector import Error
from mysql.connector import errorcode

# call back to trigger when a message is received
while True:
    def on_message_received(topic, payload, dup, qos, retain, **kwargs):
        obj = json.loads(payload)
        print(obj)
        temp = (obj['Temperature'])
        print(temp)
        dt = (obj['Data Hora'])
        print(dt)
        print("Received message from topic '{}': {} - {}".format(TOPIC, temp, dt))
        try:
            connection = mysql.connector.connect(host='localhost',
                                                 database='mqtt',
                                                 user='root',
                                                 password='')
            mySql_insert_query = f"INSERT INTO temperature (temperature, datetime) VALUES ({temp},'{dt}')"
            cursor = connection.cursor()
            cursor.execute(mySql_insert_query)
            connection.commit()
            print(cursor.rowcount, "Data entered successfully")
            cursor.close()
        except mysql.connector.Error as error:
            print("Database failure".format(error))
        finally:
            if (connection.is_connected()):
                connection.close()
                print("Closed connection")
        time.sleep(10)
    # subscribe to topic
    subscribe_future, packet_id = mqtt_connection.subscribe(
        TOPIC,
        qos=mqtt.QoS.AT_LEAST_ONCE,
        callback=on_message_received)
    print(f'Subscribed to {TOPIC}')

    # result() waits until a result is available
    subscribe_result = subscribe_future.result()
    time.sleep(10)
