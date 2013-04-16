Really? Another JSON library?
=============================

Yup, and here is why: all other JSON libraries either use STL, their own internal container classes, or a combination of both. But I don't want to use use STL and I already have my own container classes. Well then, here's JsonByoc - bring your own container.

Of course there is some work involved to connect JsonByoc to your containers. I hope I made it easy enough, and there is a fully functional example in main.cpp.

The idea is that you provide an implementation of an abstract interface. There are five methods to implement: an AddProperty get called for each name/value pair, a BeginObject/EndObject combo, same again for BeginArray/EndArray. (Actually, there is a sixth on: an error handler. I lied)

JsonByoc will parse the input text, and feed the text in handy little snippets to your code. You then put them into your own hash table, linked list or tree container.

And JsonByoc works backwards as well. If you can enumerate the data in your containers and call AddProperty, BeginObject/EndObject and BeginArray/EndArray, JsonByoc will put it together into valid JSON.
