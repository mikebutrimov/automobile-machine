# automobile-machine

For non DUE* boards (project_am_tests) it depends on this fork of stl library:
http://andybrown.me.uk/2011/01/15/the-standard-template-library-stl-for-avr-with-c-streams/

DO NOT FORGET to put it into your arduino ide hardware/tools/avr/lib/avr/include
or somewhere else.

project_am_due_alpine part however works with standart ETL library, available in arduino ide repo. 



###What it is?
It is couple of arduino sketches and android app. All in very deep alpha stage.
General purpose is to orgonize connection between vehicle (by vehicle i mean automobile, car or whatever word do you prefer)
and android device. It means that car must be able to control android (like Play, Pause, FForward music and so on), android must control car media, emulating dashboard buttons and so on.
 
Also it is possible to control some Alpine sound proccessors (a little part of command send and asck is wotking now, stay tuned).

key parts:
 - project_am_due_alpine: arduino sktech, which linked car, android and alpine sound processor
 - project_am_tests_android - android app with a service
 - 
