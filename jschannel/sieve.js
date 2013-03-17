// simplitic implementation of channel
var Channel = function() {
    var saved_value = null;
    var saved_receiver = null;
    var saved_sender = null;
    var try_push = function() {
        if (saved_value && saved_receiver) {
            var r = saved_receiver;
            var v = saved_value;
            var s = saved_sender;
            saved_value = saved_receiver = saved_sender = null;
            r(v);
            s();
        }
    };
    return {
        recv: function(recv_callback) {
            if (saved_receiver) {
                console.log("warn: more than on receiver?");
            }
            saved_receiver = recv_callback;
            try_push();
        },
        send: function(value, send_callback) {
            if (saved_value) {
                console.log("warn: more than one send?");
            }
            saved_value = value;
            saved_sender = send_callback;
            try_push();
        }
    }
};


// And the sieve code:
var counter = function(c) {
    var i = 2;
    var inc = function() {
        i = i + 1;
        return c.send(i, inc);
    };
    return c.send(i, inc);
};

var filter = function(prime, recv, send) {
    var r = function(i) {
        if (i % prime) {
            return send.send(i, function() {
                return recv.recv(r);
            });
        } else {
            return recv.recv(r);
        }
    };
    return recv.recv(r)
}

var sieve = function(primes) {
    var count = Channel();
    counter(count);

    var new_prime = function(prime) {
        primes.send(prime, function() {
            var newc = Channel();
            filter(prime, count, newc);
            count = newc;
            return count.recv(new_prime);
        });
    }
    return count.recv(new_prime);
};

var primes = Channel();
sieve(primes);

var i = 10;
var f = function(prime) {
    console.log(prime);
    i -= 1;
    if (i)
        primes.recv(f);
};
primes.recv(f);
