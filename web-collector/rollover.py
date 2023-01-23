import sys
import os
import json
import time
import datetime
import pytz

import duckdb
import pandas as pd

con = duckdb.connect(database=':memory:')
tz = pytz.timezone('US/Pacific')
view = """
create view all_data as
with 
cut as (select * from 'data/cutoff.csv'),
archive_data as (
    select device, time, sensor, value from 'archive.parquet', cut where time <= cut.cutoff),
recent_data as (
    select device, time, sensor, value from recent, cut 
    where time > cut.cutoff and time <= {last_cut})
select * from archive_data 
union all 
select * from recent_data
"""
copy = """
copy all_data to 'update.parquet' (FORMAT PARQUET, CODEC ZSTD)
"""

last_cut = time.time()
print(last_cut)

with open('sensor.log') as f:
    x = pd.DataFrame(json.loads(line.strip(str(chr(0)))) for line in f)
recent = pd.melt(x, 
        id_vars=['device', 'time'], 
        value_vars=['wifi', 'rco2', 'pm02', 'tvoc', 'atmp', 'rhum'],
        var_name='sensor')
print("read {} rows of recent data".format(recent.shape))

con.execute(view.format(last_cut=last_cut))
print("created view")

df = con.execute(copy).fetchdf()
print("wrote output file")

os.rename('archive.parquet', 'old.parquet')
os.rename('update.parquet', 'archive.parquet')
with open('data/cutoff.csv', 'w') as f:
    f.write("cutoff\n{last_cut}\n".format(last_cut=last_cut))
print("moved files and adjusted cutoff")

line_count = 0
v = dict(time = last_cut)
with open('remainder.log', 'w') as out:
    with open('sensor.log') as f:
        for line in f:
            try:
                v = json.loads(line)
            except:
                print(f"Problem on line {line_count}: {line}")
            line_count =  line_count + 1
            if v['time'] >= last_cut:
                out.write(line)
os.rename('sensor.log', 'old.log')
os.rename('remainder.log', 'sensor.log')

final_cut = v['time']
with open('sensor.log', 'a') as out:
    with open('old.log') as f:
        for line in f:
            v = json.loads(line)
            if v['time'] > final_cut:
                out.write(line)
                print("found interstitial data")
