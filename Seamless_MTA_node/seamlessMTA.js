var express = require('express');
var server = express();
var bodyparser = require('body-parser');
server.use(bodyparser.urlencoded({
    extended: true
}));
server.use('/', express.static('public'));


function checkTimeToGo(request, response) {
    var timeToStation, advanceTime;
    var upComingTrainNum = 10; // numbers of upcoming trains you want to calculate.

    // make an array of the coming trains.
    var timeTable = [];
    for (var i = 0; i < upComingTrainNum; i++) {
        var trainComingTime = Math.floor(Math.random() * 21);
        timeTable[i] = trainComingTime;
    }

    // check the request method here.
    if (request.method == "GET") {
        // name = request.query.name;
        // age = request.query.age;
        // if got a GET request, parse it here.
    } else if (request.method == "POST") {
        timeToStation = request.body.timeToStation;
        advanceTime = request.body.advanceTime;
    }
    var responseSting = "";
    var timeAll = parseInt(timeToStation) + parseInt(advanceTime);


    var newTable = []; // create a new arry to hold the new elements.

    for (var i = 0; i < timeTable.length; i++) { // push the value to the new array
        if (timeTable[i] > timeAll) {
            newTable.push(timeTable[i]);
        }
    }

    console.log("Next coming train in " + newTable[0] + " mins");
    var trainTimeString = "<p> The next train is coming in " + newTable[0] + " mins.</p> \n";
    var timeAvaliable = parseInt(newTable[0]) - parseInt(timeAll);
    responseSting = "<p>" + "you got " + timeAvaliable + " mins to leave home.</p> \n";
    responseSting += "<p> Start counting now.</p> \n";

    response.writeHead(200, {
        "Content-Type": "text/html"
    });

    // write and end the response here.
    response.write(trainTimeString);
    response.write(responseSting);
    response.end();
}

server.listen(8080);
console.log("server is listening on port 8080");

// handle the post request here.
server.post('/check', checkTimeToGo);