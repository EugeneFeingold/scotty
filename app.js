var http = require('http'),
	net = require('net'),
	app = http.createServer(handler),
	app2 = http.createServer(handler2),
	io = require('socket.io').listen(app),
	fs = require('fs'),
	connection, client, stopped, forceWarning, forceError;

	app.listen(8888); // this one for the browser
	app2.listen(8889); // this one for jenkins or simple url endpoint
	
var initilized = false;
var counter = 0;
var healthChecks = [];

var prodInfo = [
	"http://console.flite.com/health.jsp", // console
	"http://r.flite.com/syndication/health.jsp", //synd
	"http://t.flite.com/t/health.jsp", // metrics
	"http://api.flite.com/rr/health.jsp", //rr
	"http://anodejs01.ec2.flite.com:8339/health.jsp", //realtime
	"http://p-origin.flite.com/p/health.jsp", //proxy
	"http://data.widgetserver.com/data-api/health.jsp", //data api	
	"", // empty
	"", //empty
	"skip" //skip
]
var stageInfo = [
	"", //empty
	"", //empty
	"http://stageapp01.ec2.flite.com:8110/data-api/health.jsp", //data api
	"http://stageapp01.ec2.flite.com:8190/p/health.jsp", //proxy
	"http://stageapp01.ec2.flite.com:8339/health.jsp", //realtime
	"http://sreprest01.ec2.flite.com:8160/rr/health.jsp", //rr
	"http://smet01.ec2.flite.com:8100/t/health.jsp", // metrics
	"http://stageapp01.ec2.flite.com:8090/syndication/health.jsp", //synd
	"http://stageapp01.ec2.flite.com:8080/health.jsp", // console
	"skip" //skip
]
var qaInfo = [
	"http://console.flite.com/health.jsp", // console
	"http://r.flite.com/syndication/health.jsp", //synd
	"http://t.flite.com/t/health.jsp", // metrics
	"http://api.flite.com/rr/health.jsp", //rr
	"http://anodejs01.ec2.flite.com:8339/health.jsp", //realtime
	"http://p-origin.flite.com/p/health.jsp", //proxy
	"http://data.widgetserver.com/data-api/health.jsp", //data api	
	"", // empty
	"", //empty
]

for (var i = 0; i < prodInfo.length; i++) {
	healthChecks.push({
		url: prodInfo[i]
	});
	console.log(healthChecks.length + ". " + prodInfo[i]);
}

for (var i = 0; i < stageInfo.length; i++) {
	healthChecks.push({
		url: stageInfo[i]
	});
	console.log(healthChecks.length + ". " + stageInfo[i]);
}
for (var i = 0; i < qaInfo.length; i++) {
	healthChecks.push({
		url: qaInfo[i]
	});
	console.log(healthChecks.length + ". " + qaInfo[i]);
}

function handler2(req, res) {
	console.log("resetting client from handler2");
	var url = require('url');
	var url_parts = url.parse(req.url, true);
	var query = url_parts.query;
	console.log(query);
	resetAll();
	getHealthChecks();
	res.end("welcome to scotty! " + JSON.stringify(query) + "\n");
}
function handler(req, res) {
	fs.readFile(__dirname + '/index.html', function(err, data) {
		if (err) {
			res.writeHead(500);
			return res.end('Error loading index.html');
		}

		res.writeHead(200);
		res.end(data);
	});
}
function getHealthChecks() {
	var healthCheck = healthChecks[counter]; 
	var url = healthCheck.url;
	
	console.log("url: " + url);
	
	//client.write('[[0' + counter + '000000]]\n');
	
	
	if (url == "") {
		setLight(counter, "202020", ""); // not used
		next();
		
	} else if (url == "skip") {
		setLight(counter, "000000", ""); // not used
		next();

	} else if (forceError) {
		setLight(counter, "ff0000990000", "60"); // error
		next();
		 
	} else if (forceWarning) {
		setLight(counter, "FFCC00FF9900", ""); // warning
		next();
		
	} else {
		setLight(counter, "00009999FFFF", "40"); // pending
		http.get(url, function(res) {
		    var body = '';

		    res.on('data', function(chunk) {
		        body += chunk;
		    });

		    res.on('end', function() {
		        var data = null;
				try {
					data = JSON.parse(body);
					var items = data.items;
					var okCount = 0;
					var totalItems = 0;
					for (var key in items) {
						var item = items[key];
						totalItems++;
						if (item.status == "OK") {
							okCount++;
						} 
					}
					console.log("OK: " + okCount + "  TOTAL: " + totalItems);
					if (okCount == totalItems) {
						//client.write('[[0' + counter + '00ff00]]\n'); // green
						setLight(counter, "00ff00", ""); 
					} else if (okCount == 0) {
						//client.write('[[0' + counter + 'ff000099000060]]\n'); // red
						setLight(counter, "ff0000990000", "60"); 
					} else {
						//client.write('[[0' + counter + 'ffff00FF9900]]\n'); // warning yellow
						setLight(counter, "FFCC00FF9900", "");
					}
				
				} catch (e) {
					console.log(e);
				}
				if (data != null && connection != null) {
		        	console.log("Got response: ", data);
					connection.emit('gotData', data);
				
				}
				next();
		    });
		
		}).on('error', function(e) {
		    console.log("Got error: ", e);
			setLight(counter, "ff0000990000", "60"); //client.write('[[0' + counter + 'ff000099000060]]\n');
			next();
		});
	}
}
function setLight(index, color, rate) {
	var index = (index < 10) ? "0" + index : index;
	var rate = rate || "";
	client.write('[[' + index + color + rate + ']]\n');	
}
function next() {
	if (counter + 1 == healthChecks.length) {
		initilized = true;
		counter = 0;
	} else {
		counter++;
	}
	if (!stopped) {
		var delay = !initilized ? 0 : 2000;
		setTimeout(function() {
			getHealthChecks();
		}, delay);
	}
}
function resetAll() {
	counter = 0;
	initilized = false;
	client.write('[[reset]]\n');
	for (var i = 0; i < healthChecks.length; i++) {
		setLight(i, "c0c0c0", "");
	}
}

client = net.connect("23", "10.1.1.230").on('data', function(data) {
     console.log('data:', data.toString());

}).on('error', function(err) {
     console.log('error:', err.message);

});

io.sockets.on('connection', function(socket) {
	socket.emit('news', {
		hello: 'world'
	});
	socket.on('testError', function(data) {
		console.log("testing error lights");
		forceWarning = false;
		forceError = true;
	});
	
	socket.on('testWarning', function(data) {
		console.log("testing warning lights");
		forceWarning = true;
		forceError = false;
	});
	
	socket.on('stop', function(data) {
		console.log("stopping");
		stopped = true;
	});
	
	socket.on('resume', function(data) {
		console.log("resuming");
		stopped = false;
		getHealthChecks();
	});		
	
	socket.on('reset', function(data) {
		console.log("reset");
		stopped = false;
		forceWarning = false;
		forceError = false;
		resetAll();
	});
	
	socket.on('setMode', function(data) {
		console.log("reset");
		stopped = true;
		forceWarning = false;
		forceError = false;
		client.write('[[' + data.mode + ']]\n');
		
	});
	
	connection = socket;
	console.log("connected");
	

	console.log("resetting: " + client);
	resetAll();
		
	getHealthChecks();
});
