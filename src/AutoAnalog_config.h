    /*
    AutoAnalogAudio streaming via DAC & ADC by TMRh20
    Copyright (C) 2016  TMRh20 - tmrh20@gmail.com, github.com/TMRh20

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */
    
    #include <Arduino.h>

  #if defined  (ARDUINO_ARCH_SAM)
  
    #define MAX_BUFFER_SIZE 256
  
  #else    
  
    #define MAX_BUFFER_SIZE 32
    #define DAC0_PIN 9
    #define DAC1_PIN 10
  
  #endif
  
    #define DEFAULT_FREQUENCY 16000
    