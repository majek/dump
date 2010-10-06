#!/bin/bash
ERL_TOP=/home/majek/.erlang-R13B03/lib/erlang; 
DIALYZER=$ERL_TOP/../../bin/dialyzer

if [ 0 == 1 ]; then
    time $DIALYZER \
            --build_plt \
            --output_plt /home/majek/.dialyzer_plt_R13B03 \
            -r \
            $ERL_TOP/lib/kernel*/ebin
fi

foo(){
    time $DIALYZER \
        --add_to_plt \
        --plt /home/majek/.dialyzer_plt_R13B03 \
        --output_plt /home/majek/.dialyzer_plt_R13B03 \
        -r $1
}

foo $ERL_TOP/lib/stdlib*/ebin
foo $ERL_TOP/lib/mnesia*/ebin       
foo $ERL_TOP/lib/erts*/ebin         
foo $ERL_TOP/lib/crypto*/ebin       
foo $ERL_TOP/lib/sasl*/ebin         
foo $ERL_TOP/lib/dialyzer-*/ebin    
foo $ERL_TOP/lib/os_mon*/ebin       
foo $ERL_TOP/lib/inets*/ebin        
foo $ERL_TOP/lib/ssl*/ebin          
foo $ERL_TOP/lib/hipe*/ebin         
foo $ERL_TOP/lib/tool*/ebin

