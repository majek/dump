<style>
.gist-meta {display: none;}
.gist-table {border: 0px;}
.gist-table td {border: 0px;}
</style>


CoffeeScript quirks
===================

<br>

Okay, okay. You can say that I belong to the YCombinator crowd and
sometimes I do try bleeding edge technologies. This time I gave
[CoffeeScript](http://jashkenas.github.com/coffee-script/) a shot. If
you don't know what it is, here's how the authors describe it:

> CoffeeScript is a little language that compiles into JavaScript.


Recently I found myself writing more and more JavaScript, a big chunk
of my work is server-side code for Node.js. Experimenting with
CoffeeScript in that environment sounds reasonably.

Below are my observations, bear in mind that they are written from a
novice point of view.


Compilation + installation
--------------------------

CS works great out of the box. To install it type:

    npm install coffeescript

and that's it. To compile CoffeeScript to JavaScript you may run:

    coffee -o lib/ -c src/*.coffee

(I use `inotifywait` to automatically recompile files when they're
modified, [full script](https://github.com/majek/sockjs-node/blob/master/run.sh))

And here goes a first surprise, even though `coffee` is written in
JavaScript, it's really fast! No need to maintain a makefile, you can
simply recompile everything every time. My eleven files with 1.1k LoC
of CS compile in 240 ms. Half of this time is warming up the compiler,
it will take me quite some time to write enough code to notice the
compilation time.


Parenthesis
-----------

My major complain about CS is the confusion caused by parenthesis-less
syntax. Let's start with a basic example:

<table class="gist-table"><tr><td valign=top>
<script src="https://gist.github.com/1133890.js"> </script>
</td><td valign=top>
<script src="https://gist.github.com/1133891.js"> </script>
</td></tr></table>

That was quite simple, without parameters you need parenthesis to call
a function. But if you dig a bit deeper, it get's messy:

<table class="gist-table"><tr><td valign=top>
<script src="https://gist.github.com/1133903.js"> </script>
</td><td valign=top>
<script src="https://gist.github.com/1133904.js"> </script>
</td></tr></table>

Ranges are weird
----------------

I don't like the generated code for ranges, for example:

<table class="gist-table"><tr><td valign=top>
<script src="https://gist.github.com/1133850.js"> </script>
</td><td valign=top>
<script src="https://gist.github.com/1133851.js"> </script>
</td></tr></table>

Well, that could be a bit simpler. Similar example:

<table class="gist-table"><tr><td valign=top>
<script src="https://gist.github.com/1133827.js"> </script>
</td><td valign=top>
<script src="https://gist.github.com/1133824.js"> </script>
</td></tr></table>

In my opinion ranges syntax is quite confusing, can you guess what's
the interpretation of the following lines?

<script src="https://gist.github.com/1133858.js"> </script>

Classes and inheritance
-----------------------

My
