import serial, time
from time import gmtime, strftime
from datetime import datetime
import sys, math
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS

# pip install pyserial influxdb-client

# hmm secret store? whats that? :p
token = "GqZaCzMv1Hl0hRd4HHDKA5YhHo4oyNYpLfkk8q1TnnBHq1IAH2TG-GRTjojxRYRiPR8JKLCuSVgpDEu9mb5taA=="
org = "default_org"
bucket = "iot"
client = InfluxDBClient(url="http://192.168.1.5:3002", token=token, org=org)
write_api = client.write_api(write_options=SYNCHRONOUS)

#Collect data
ser = serial.Serial('/dev/ttyUSB0', baudrate=115200)

#Configure the labels for the database
device = "rf_mini_gas"
location = "gas_cellar"

try:
    while True:
        now = datetime.now()
        iso = datetime.utcnow()
        data = ser.readline()

        if b"," not in data:
            print("non-measurement-output: ", data)
            continue
        try:
            magx, magy, magz, temp, volt, rssi = data.decode().strip().split(",")
        except:
            print(f"Error parsing {data}")
            continue
        
        magx, magy, magz, rssi = map(int, [magx, magy, magz, rssi])
        temp, volt = map(float, [temp, volt])

        mmag = math.sqrt(magx*magx+magy*magy+magz*magz)
        
        # Print for debugging, uncomment the below line
        #print(magx, magy, magz, temp, volt, rssi)
        print(f"[{iso}] MX: {magx}, MY: {magy}, MZ: {magz}, |M|: {mmag:.2f}, temp: {temp}, volt: {volt}, rssi: {rssi}")
        
        points = []
        points.append(Point("environment").tag("location", location).tag("device", device)
            .field("temperature", temp))
        points.append(Point("meta").tag("location", location).tag("device", device)
            .field("rssi", rssi))
        points.append(Point("meta").tag("location", location).tag("device", device)
            .field("battery_voltage", volt))
        points.append(Point("energy_usage").tag("location", location).tag("device", device)
            .field("magx", magx).field("magy", magy).field("magz", magz).field("mag_mag", mmag))
        write_api.write(bucket, org, record=points)
        

except KeyboardInterrupt:
    write_api.__del__()
    client.__del__()
    pass