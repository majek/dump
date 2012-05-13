from cluster import AtLeastOneCluster

class Cluster(AtLeastOneCluster):
    def on_setup(self, client, _setup_done):
        self._setup_done = setup_done
        # Called every time a successful connection to the node was
        # made. Set up queues and exchanges here.
        client.queue_declare(queue='q', auto_delete=True, durable=False,
                             callback=self.on_setup_done)


cluster = Cluster()
cluster.add_node('amqp://localhost:5672')


cluster.publish()
