""" Connecting to multimeter esp32 and running Bokeh graph"""

from datetime import datetime
from threading import Thread
import asyncio
from functools import partial
from bokeh.plotting import figure, curdoc
from bokeh.models import ColumnDataSource, HoverTool

import aioesphomeapi
import yaml
import os

source1 = ColumnDataSource(data={"x": [], "y": []})
source2 = ColumnDataSource(data={"x": [], "y": []})

hover_tool = HoverTool(
    tooltips=[("Time", "@x{%H:%M}"), ("Temperature", "$y")],
    formatters={"@x": "datetime"},
    # display a tooltip whenever the cursor is vertically in line with a glyph
    mode="vline",
)

p = figure(
    width=1200,
    height=600,
    x_axis_type="datetime",
    x_axis_label="Time",
    y_axis_label="Temperature",
    tools=[hover_tool],
)
p.line(x="x", y="y", color="blue", line_width=2, legend_label="Probe", source=source1)
p.line(x="x", y="y", color="red", line_width=2, legend_label="Ambient", source=source2)

doc = curdoc()
doc.add_root(p)

loop = asyncio.new_event_loop()


async def update(x_value, y_value, y2_value):
    source1.stream({"x": [x_value], "y": [y_value]})
    source2.stream({"x": [x_value], "y": [y2_value]})


async def main():
    with open(os.join(os.path.dirname(__file__), "../secrets.yaml")) as secrets_file:
        secrets = yaml.safe_load(secrets_file)
    cli = aioesphomeapi.APIClient("121gw.local", 6053, None, noise_psk=secrets["app_key"])

    await cli.connect(login=True)

    def service_callback(service_event):
        """Print the state changes of the device.."""
        mode = int(service_event.data["mode"])
        if service_event.is_event and mode == 5:
            print(service_event.data)
            doc.add_next_tick_callback(
                partial(
                    update,
                    x=datetime.now(),
                    y=int(service_event.data["value"]) / 10,
                    y2=int(service_event.data["sub_value"]) / 10,
                )
            )

    await cli.subscribe_service_calls(service_callback)


def blocking_main():
    # loop = asyncio.get_event_loop()
    asyncio.set_event_loop(loop)
    try:
        asyncio.ensure_future(main())
        loop.run_forever()
    except KeyboardInterrupt:
        pass
    finally:
        loop.close()


thread = Thread(target=blocking_main, daemon=True)
thread.start()
