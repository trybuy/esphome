""" Connecting to camera esp32 and running Bokeh graph"""

from datetime import datetime
from threading import Thread
import asyncio
from functools import partial
from bokeh.plotting import figure, curdoc
from bokeh.models import ColumnDataSource, HoverTool

import aioesphomeapi
import yaml
import os

# source1 = ColumnDataSource(data={"x": [], "y": []})
# source2 = ColumnDataSource(data={"x": [], "y": []})

# hover_tool = HoverTool(
#     tooltips=[("Time", "@x{%H:%M}"), ("Temperature", "$y")],
#     formatters={"@x": "datetime"},
#     # display a tooltip whenever the cursor is vertically in line with a glyph
#     mode="vline",
# )

# p = figure(
#     width=1200,
#     height=600,
#     x_axis_type="datetime",
#     x_axis_label="Time",
#     y_axis_label="Temperature",
#     tools=[hover_tool],
# )
# p.line(x="x", y="y", color="blue", line_width=2, legend_label="Probe", source=source1)
# p.line(x="x", y="y", color="red", line_width=2, legend_label="Ambient", source=source2)

# doc = curdoc()
# doc.add_root(p)

loop = asyncio.new_event_loop()


# async def update(x_value, y_value, y2_value):
#     source1.stream({"x": [x_value], "y": [y_value]})
#     source2.stream({"x": [x_value], "y": [y2_value]})


async def main():
    secret_file_path = os.path.join(os.path.dirname(__file__), "../secrets.yaml")
    with open(secret_file_path) as secrets_file:
        secrets = yaml.safe_load(secrets_file)
    cli = aioesphomeapi.APIClient("esp32-cam.local", 6053, None, noise_psk=secrets["api_key"])

    await cli.connect(login=True)

    # List all entities of the device
    entities = await cli.list_entities_services()
    for e in entities:
        for z in e:
            print(z)

    def service_callback(service_event):
        """Print the state changes of the device.."""
        print(service_event)

    await cli.subscribe_service_calls(service_callback)

    def change_callback(state):
        if type(state) == aioesphomeapi.CameraState:
            try:
                with open(os.path.join(os.path.dirname(__file__), 'x.jpg'),'wb') as out:
                    out.write(state.data)
                print("Image written", state.key)
            except Exception as e:
                print(e)

    # Subscribe to the state changes
    await cli.subscribe_states(change_callback)

    resp = await cli.request_single_image()
    print(resp)


def blocking_main():
    asyncio.set_event_loop(loop)
    try:
        asyncio.ensure_future(main())
        loop.run_forever()
    except KeyboardInterrupt:
        pass
    finally:
        loop.close()


#thread = Thread(target=blocking_main, daemon=True)
#thread.start()
blocking_main()
