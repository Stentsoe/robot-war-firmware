.. _robot_wars_gateway:

Robto wars firmware
###################

.. contents::
   :local:
   :depth: 2




Samples
********





Installing
**********


.. msc::
   hscale = "0.5";

   a,b,c;

   a->b [ label = "ab()" ] ;
   b->c [ label = "bc(TRUE)"];
   c=>c [ label = "process()" ];