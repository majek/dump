
import puka
import collections
import uuid

class AtLeastOneCluster:
    max_timeout = 10000
    def __init__(self):
        self.nodes = []
        self.unsent_messages = []
        self.messages = collections.OrderedDict()

    def on_setup(self, client, setup_done):
        setup_done()

    def add_node(self, amqp_url):
        self.nodes.append( Node(amqp_url, self.max_timeout) )

    def del_node(self, amqp_url):
        node = [n for n in self.nodes if n.amqp_url == amqp_url][0]
        self.nodes.remove(node)
        node.mark_dead(reconnect=False)

    def publish(self, *args, **kwargs):
        msg_id = uuid.uuid4()
        self.unsent_messages.append(msg_id)
        self.mesages[msg_id] = Message(msg_id, args, kwargs)
        self.maybe_flush()

    def maybe_flush(self):
        nodes = [n for n in self.nodes if n.status is OPEN]
        if not nodes or not self.unsent_messages:
            return
        node = nodes[0]
        while self.unsent_messages:
            msg_id = self.unsent_messages.pop(0)
            node.publish(self.messages[msg_id])

DEAD=0
CONNECTING=1
SETTINGUP=2
OPEN=3
WAITING=4

class Node:
    status = DEAD
    client = None

    def __init__(self, amqp_url, max_timeout):
        self.amqp_url = amqp_url
        self.max_timeout = max_timeout
        self.do_connect()

    def do_connect(self):
        self.status = CONNECTING
        self.messages = collections.OrderedDict()
        self.client = puka.Client(self.amqp_url, pubacks=True)
        self.client.connect(callback=self.on_connect)

    def on_connect(self):
        self.status = SETTINGUP
        self.timeout = 2
        self.XXXX.on_setup(self, self.on_setup_done)

    def on_setup_done(self):
        self.status = OPEN
        self.XXXX.maybe_flush()

    def mark_dead(self):
        self.status = WAITING
        self.client.close()
        self.client = None

        self.XXXX.nack()
        to, self.timeout = self.timeout, min(self.timeout**2,
                                             self.max_timeout)
        self.XXXX.add_timer(to, self, self.do_connect)

    def close(self):
        if self.client:
            self.client.close()

    def publish(self, msg):
        def on_publish(_promise, _result):
            self.on_publish(msg)
        pub_kwargs = msg.kwargs.copy()
        pub_kwargs['callback'] = on_publish;
        self.client.basic_publish(*msg.args, **pub_kwargs)


Message = collections.namedtuple('Message', ['msg_id', 'args', 'kwargs'])
