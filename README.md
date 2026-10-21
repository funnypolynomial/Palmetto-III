# Palmetto-III
A simulation on an Arduino with touch LCD of just enough of the Palm III look and feel to resurrect an app I wrote almost 30 years ago.

I recently unearthed my old Palm III from the late 90's and it made me think of a Palm app I wrote in **1998**.  I decided to embark on a project to re-create it, in some form, on an Arduino with a touch LCD shield.  Palmetto-III ("small palm", *heh*) is the result.  Just enough infrastructure to create a facsimile of my "Count Down" app.

<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/df08f265-465d-4123-afed-9c0bc1565065" />


It has enough of the look and feel of a Palm to achieve my goal. 
**Simulation not emulation.**

[YouTube video](https://youtu.be/VJ-6srZ3FKI)

The LCD is 320x480 so the Palm's 160x160 screen fits nicely with a scale factor of x2.  The Graffiti area is proportionately a little taller. The apps form has just my program ("Count Down") and a Prefs app. The latter sets the date, time and calibrates the resistive touch ("Digitizer"):
<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/f69b4ead-c67d-4f10-9d99-537dadfd932c" />

The Calc and Find buttons do nothing. The App button just returns to the apps screen. *Menus aren't implemented* – the Menu button just brings up an About box, if applicable.
<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/e5f5419c-05fb-4eec-b9ce-9d0be1ad3a26" />

I couldn't get "Applications", "Calculator" etc labels to look decent at this resolution so omitted them.

Countdown shows days, hours, minutes and seconds counting up from, or down to, up to four events
<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/e17ae21b-f08f-41e0-8609-24430dec31a0" />

Clicking on an event control brings up a details form:
<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/82cecf6f-372f-4424-9842-1952c50f5e18" />

*There is no Graffiti support!* Instead the event name is entered using a small keyboard (optionally) labelled with Graffiti glyphs as a homage. Text entry is basic – append and backspace only. Note that fonts are limited, to save program space, not all chars are defined. For example LargeBold only has "IPaelmot" for "Palmetto-III" on the splash/about.

The event date is entered via a small "standard" form.  
<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/3a1e31fe-ea69-4769-b07a-5a7238d02901" />

Like the Prefs app and the time-setting and Digitizer forms, this is my own simplified version and does not match the real thing. Other Prefs, like data and time format, are compile-time #defines in Prefs.h.

The event time form has a None option:
<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/cb851ebe-c629-470d-ad9f-337d7a547834" />

Hitting Menu brings up Count Down's About:
<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/3ff865c3-8541-4ba1-b85d-10ee4f011e3d" />
The Count Down logic has been written from scratch, I paid little attention to my original code, other than the resources. I'm not faking original API calls.

I came up with an identity for my Palm developments. The *triquetral* is a bone in the hand, if not actually the palm, and has *three* in the name.  My younger self was inordinately pleased with this.

The code includes app launching and lifetime logic, there's a hierarchy of form and control classes. Authentic fonts and bitmaps are encoded as PROGMEM data extracted using Python scripts (see the Assets folder).

The hardware is a stack:
1. the 320x40 resistive-touch LCD shield plugged into
2. a blank stackable shield with a DS1307 real-time clock and a passive piezo buzzer (for an optional "click" on stylus down) plugged into
3. an Arduino Uno (or a Mega)
<img width="2048" height="1536" alt="image" src="https://github.com/user-attachments/assets/85653874-6279-45d5-93af-97593bbc0ad1" />


I prototyped a little stand for the Uno to hold it upright:
<img width="2048" height="1536" alt="image" src="https://github.com/user-attachments/assets/29e1d1da-e825-4dc9-86c5-f4da56f3bdba" />

<img width="1368" height="1824" alt="image" src="https://github.com/user-attachments/assets/a78a01c0-2e10-4568-89f3-0ec21154d627" />


There is more detail in a bulk comment at the top of Palmetto-III.ino and additional photos on [Flickr](https://www.flickr.com/gp/funnypolynomial/75ESyw78ZU).

Hackaday project [here](https://hackaday.io/project/207132-palmetto-iii)

