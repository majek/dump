#!/usr/bin/env escript
%%! -pz ./amqp_client ./rabbit_common ./amqp_client/ebin ./rabbit_common/ebin

-include_lib("amqp_client/include/amqp_client.hrl").

-define(COUNT, 20000).

main(_) ->
    {ok, Connection} = amqp_connection:start(network,
                                             #amqp_params{host = "localhost"}),
    {ok, Channel} = amqp_connection:open_channel(Connection),
    {ok, Channel2} = amqp_connection:open_channel(Connection),

    #'exchange.declare_ok'{} = amqp_channel:call(Channel, #'exchange.declare'{
                                                   exchange = <<"test_direct">>,
                                                   type = <<"direct">>}),

    #'queue.declare_ok'{} = amqp_channel:call(Channel,
                                              #'queue.declare'{
                                                queue = <<"hello">>,
                                                auto_delete = true}),

    #'queue.bind_ok'{} = amqp_channel:call(Channel,
                                           #'queue.bind'{
                                             exchange = <<"test_direct">>,
                                             queue = <<"hello">>,
                                             routing_key = <<"hello">>}),

    amqp_channel:subscribe(Channel2, #'basic.consume'{queue = <<"hello">>,
                                                     no_ack = true}, self()),
    receive
        #'basic.consume_ok'{} -> ok
    end,

    io:format("Start~n"),
    T0 = now(),

    [amqp_channel:cast(Channel,
                      #'basic.publish'{
                        exchange = <<"test_direct">>,
                        routing_key = <<"hello">>},
                      #amqp_msg{payload = <<"Hello World!">>})
     || X <- lists:seq(0, ?COUNT-1)],

    loop(Channel2, ?COUNT),
    Td = timer:now_diff(now(), T0) / 1000000.0,
    io:format("Done ~.3fsec ~.3f MPS~n", [Td, ?COUNT/Td]),
    ok = amqp_channel:close(Channel),
    ok = amqp_channel:close(Channel2),
    ok = amqp_connection:close(Connection),
    ok.

loop(Channel, 0) ->
    ok;

loop(Channel, Cnt) ->
    receive
        {#'basic.deliver'{}, #amqp_msg{payload = Body}} ->
            loop(Channel, Cnt-1)
    end.

