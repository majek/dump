var express = require('express');

var app = express.createServer();

var port = process.env.VMC_APP_PORT || 9998;
console.log(' [*] Listening on 0.0.0.0:' + port);
app.listen(port, '0.0.0.0');

app.get('/', function (req, res) {
    res.sendfile(__dirname + '/index.html');
});

app.get('/set_headers.txt', function (req, res) {
    res.header('X-Fb-DeBuG', 'casing baby!');
    res.send('ok');
});
