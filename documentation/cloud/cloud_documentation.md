# Cloud Documentation

Open source free GPS Tracker softwares are provided online eg Traccar (https://www.traccar.org/)
A cloud instance can run such a software and provide Tracking capabilities to us

## Traccar Implementation
1. Using OsmAnd Protocol: See the link - https://www.traccar.org/osmand/
2. Simply need to make http get int this format - http://demo.traccar.org:5055/?id=123456&lat={0}&lon={1}&timestamp={2}&hdop={3}&altitude={4}&speed={5}
3. Challenge will be to make a functionality which will store the gps data in a sd card when no signal is available and then mass post the data to traccar server when signal/network is available
