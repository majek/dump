%% erlc test.erl && erl -noshell -run test start -run init stop;
% gc_length_1(0x7fa766b65991)=299999 1.547ms avg=16.000 dev=0.029 unsorted=299998
% gc_length_1(0x7fa766b65991)=299999 2.147ms avg=16.000 dev=0.029 unsorted=299998
% gc_length_1(0x7fa766b65991)=299999 2.097ms avg=16.000 dev=0.029 unsorted=299998
% gc_length_1(0x7fa762436049)=299999 16.447ms avg=136.000 dev=0.343 unsorted=0
% gc_length_1(0x7fa762436049)=299999 15.174ms avg=136.000 dev=0.343 unsorted=0
% gc_length_1(0x7fa762436049)=299999 15.179ms avg=136.000 dev=0.343 unsorted=0


-module(test).
-export([start/0]).
-export([loop/1]).

start() ->
    Pid = spawn(test, loop, [none]),
    Pid ! init,
    Pid ! len,
    Pid ! len,
    Pid ! len,
    Pid ! {self(), ping},
    receive
        pong -> ok
    end,
    Pid ! hibernate,
    receive
        _ -> ok
    after 1000 -> ok
    end,
    Pid ! len,
    Pid ! len,
    Pid ! len,
    Pid ! {self(), ping},
    receive
        pong -> ok
    end,
    Pid ! quit.

items(I) ->
    lists:map(fun(_) -> {{ppp,{
                    resource,<<"1">>,exchange,<<"">>},<<"aaaa">>,{content,60,
                            {'P_basic',<<"asdfghjklp">>,undefined,undefined,2,undefined,
                            undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined},
                            <<"1234567890olki">>,[<<"123212321232">>]},{{4,list_to_pid("<0.202.0>")},22771}},false}
                        end,
    lists:seq(0, I)).

avg_dev(ListOfValues) ->
    Sum = lists:sum(ListOfValues),
    SumSq = lists:sum(lists:map(fun (T) -> T*T end, ListOfValues)),
    Avg = Sum / length(ListOfValues),
    Dev = math:sqrt( (SumSq / length(ListOfValues)) - (Avg*Avg) ),
    {Avg, Dev}.

loop(Queue) ->
    receive
        init ->
            io:format("init ~n"),
            loop(queue:join(queue:new(), queue:from_list(items(300*1000))));
        len ->
            ListOfTime = lists:map(fun(_) ->
                                        T1 = erlang:now(),
                                        L = queue:len(Queue),
                                        T2 = erlang:now(),
                                        timer:now_diff(T2, T1) / 1000.0 % ms
                                    end, lists:seq(0, 100)),
            {Avg, Dev} = avg_dev(ListOfTime),
            L = queue:len(Queue),
            io:format("length: ~p  avg:~fms dev:~fms~n", [L, Avg, Dev]),
            loop(Queue);
        hibernate ->
            io:format("hibernate~n"),
            erlang:hibernate(test, loop, [Queue]);
        {From, ping} ->
            From ! pong,
            loop(Queue);
        _ -> ok
    end.

