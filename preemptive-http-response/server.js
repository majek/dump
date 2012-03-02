var express = require('express');
var net = require('net');

// 2. Express server
var app = express.createServer();

console.log(' [*] Listening on 0.0.0.0:9999' );
app.listen(9999, '0.0.0.0');

app.get('/', function (req, res) {
    res.sendfile(__dirname + '/index.html');
});

app.get('/xhr', function (req, res) {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.send('yeah!')
});


var response = function(i) {
    i = '' + i;
    return ['HTTP/1.1 200 OK',
            'Connection: keep-alive',
            'Content-Type: text/plain',
            'Access-Control-Allow-Origin: *',
            'Content-Length: ' + i.length,
            '',
            i].join('\r\n');
};



var s = net.Server();
s.listen(10000);
s.on('connection', function(conn) {
    var port = conn.remotePort;
    console.log(" [*] conn started!", port, conn.remoteAddress);

    var i = 0;
    var res = function () {
        i += 1;
        a = response(port + ':' + i);
        return a;
    }
    conn.on('data', function(d) {
        d = d.toString('utf-8');
        console.log(" [.] data         ", port, d.length);
        if (d.indexOf('GET') !== -1) {
            conn.write(res());
        }
    });
    conn.on('error', function() {
        console.log(" [*] conn closed  ", port);
    });
    conn.on('close', function() {
        console.log(" [*] conn closed  ", port);
    });
});
