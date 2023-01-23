from flask import Flask, render_template, request

import sys
import json
import time
import datetime
import pytz

import duckdb
import pandas as pd
import plotly
import plotly.express as px

app = Flask(__name__)
con = duckdb.connect(database=':memory:')
tz = pytz.timezone('US/Pacific')
query = """
with 
cut as (select * from 'data/cutoff.csv'),
archive_data as (select * from 'archive.parquet', cut where sensor = '{sensor}' and time <= cut.cutoff),
recent_data as (select * from recent, cut where sensor = '{sensor}' and time > cut.cutoff),
all_data as (select * from archive_data union all select * from recent_data),
core as (
  select value, time, location, all_data.device device,
         (time - {t0})/24.0/3600.0 as day, (time - {today})/3600 as hour
  from all_data join 'data/locations.csv' using (device)
  where time >= t0 and time < t1 and value != -1 and value != 513 and value != 514 and value < 17000
  order by time, device)
select value, location, day, hour
from core
where time > {now} - 6*3600
"""

@app.route("/air/sensors/<device>/data", methods=['GET', 'POST'])
def post_data(device):
    data = json.loads(request.get_data().decode())
    data['device'] = device
    data['time'] = time.time()
    print(data)
    sys.stdout.flush()
    with open("sensor.log", "a") as f:
        f.write(json.dumps(data))
        f.write("\n")
    return f"data from {device}".format(device=device)

@app.route("/plot/<sensor>")
def plot(sensor):
    t0 = datetime.datetime.now(tz).replace(day=1,hour=0,minute=0,second=0,microsecond=0).timestamp()
    today = datetime.datetime.now(tz).replace(hour=0,minute=0,second=0,microsecond=0).timestamp()

    legal = ['wifi', 'rco2', 'pm02', 'tvoc', 'atmp', 'rhum']
    if not sensor in legal:
        return f"invalid sensor, wanted one of {legal}".format(legal=legal), 403

    with open('sensor.log') as f:
        x = pd.DataFrame(json.loads(line.strip(str(chr(0)))) for line in f)
    recent = pd.melt(x, 
            id_vars=['device', 'time'], 
            value_vars=['wifi', 'rco2', 'pm02', 'tvoc', 'atmp', 'rhum'],
            var_name='sensor')
    df = con.execute(query.format(now=time.time(), sensor = sensor, today = today, t0 = t0)).fetchdf()
    locations = con.execute("select location, color from 'data/locations.csv'").fetchdf()
    color_map = dict(x for x in locations.itertuples(index=None, name=None))
    fig = px.scatter(df, x='hour', y='value', color='location', color_discrete_map=color_map)
    fig.update_xaxes(tickfont=dict(size=20))
    fig.update_yaxes(tickfont=dict(size=20))
    fig.update_layout(font=dict(size=24))

    graphJSON = json.dumps(fig, cls=plotly.utils.PlotlyJSONEncoder)
    return render_template('plot.html', graphJSON=graphJSON)

