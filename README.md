senior-project-aptt-code
========================

Senior Project - Code to drive my Automatic Tracking and Pointing Telescope (APTT)



This was my senior project (2009 EE). You can read more about the project here: http://seiferteric.com/?p=322

This project ues a PIC18F2550 microcontroller. It controlled a telescope to automatically locate and track stars.

It had the following features:

1) USB Control
2) GPS for time and location
3) Solid state magnometer for orientation
4) (Wired) remote control with LCD display



While this is admittedly not great code, probably the most interesting code to look at is the calculation of the position of the stars in the sky based on your location, the current time, and your orientation (from north)

