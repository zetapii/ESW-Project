# PID-control-of-DC-Motor
### To start the server - `cd backend; npm start` and then go to `localhost:{PORT}` to go to the website

## Directory structure - 
* backend - contains the whole dashboard created with Express JS. The static folder of the backend contains all the static pages that website can go to. The website uses Redis as its database which is all done in `backend/sample.js`.
* pid - contains the Arduino code required to run the PID controller and has to uploaded to the arduino
* CameraWebServer - contains the code to run the ESP-32 Cam and streams the video on local area network.

The website is globally hosted on [this link](https://pid-control-of-dc-motor-production.up.railway.app/) via Railway.
