Bring-Your-Own-Container JSON Parser for C++
============================================

Most (perhaps all) C++ JSON libraries either use STL or their own internal container classes. But what if you don't want to use use STL and you already have your own container classes? That's JsnParse.

JsnParse will parse any valid JSON text, and feed the text in handy little snippets to your code, so your code can build the data using your own containers.

For example, if this is the JSON text:

```json
{
  "make": "Bugatti",
  "model": "Veyron",
  "specs": {
    "cylinders": 16,
    "mph": 253
  }
}
```

When parsing the above text, your code will receive callbacks along the lines of the following *pseudo-code:*

```
BeingObject();
AddString( "make", "Bugatti" );
AddString( "model", "Veyron" );
BeginObject( "specs" );
AddNumber( "cylinders", 16 );
AddNumber( "mph", 253 );
EndObject();
EndObject();
```

Also, JsnParse contains the essentials to write data from your own classes into valid JSON text, with or without pretty printing, in escaped or unescaped UTF-8 formats.

There is a fully functional example in main.cpp.
