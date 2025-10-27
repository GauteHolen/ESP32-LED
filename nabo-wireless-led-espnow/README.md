# ESPNOW Receiver and LED processor

This directory is responsible for the rendering of led pixels.

## Global datastructures

There are 3 global data structures that are threadsafe through mutex and accessed by multiple processes:
1) led_pixel_t led_pixels [led_pixel.h](main/led_pixel.h): A struct containing rbg values of multiple layers i.e. foreground, background, trail etc
2) state_t state [state.h](main/state.h): A struct containing static values like color, diffusion constants or other mathematical constants impacting the processing of the pixels. These are all unsigned 8-bit integers.
3) received_data_t g_received_data [espnow_receiver.h](main/espnow_receiver.h): Contains dynamic values of trigger events. Once an event has been triggered the respective values are set to zero until a new event is triggered. Any nonzero value triggers a new event. The value of an event is often the velocity of a note, which is in the range 0-127

## ESP-NOW payload

The ESP receives a data signal over ESP NOW defining the state of the system, and will process and render pixels accordingly. The data is broadcast over ESP NOW, where each ESPs MAC ADDRESS has an assigned fixture ID. 

Each ESP's fixture data is strctured like: 

* A list of 8 bit values for the state.
* A list of trigger values for the event triggers

At the broadcasted message has

* 4-bit Magic word
* 8-bit Sequence increment (to track lost or out-of-sync payloads)
* Fixture 1 data, Fixture 2 data, Fixture 3 data, ... Fixture N data

The max message size is approximately 1400 bits, such that with 20 values and 10 triggers we can have 50+ ESPs per broadcast. 

The payload is defined [message_config.h](main/message_config.h) and needs to correspond exactly to the structure in the sender code. 



## Parallel processes
There are 4 parallel tasks running at any time, managed with the FreeRTOS library, they all run at different rates:
1) Receiver loop: Reads incoming ESP NOW message and writes data to the state struct and arms trigger events
2) Main loop: Waits for trigger events and starts events (sets pixel values)
3) Pixel processing loop: Runs a number of processing on inter-dependent layers of rgb arrays
4) LED pixel refersh loop: Reads from the rbg layers, sums them and generates the LED data signal for each pixel.

## Pixel processing and simulations
Conveniently, but not coincidentally, 1D simulations and numerical schemes modelling natural phenomena are highly optimized and perfect for processing 1D led pixel strips. 

### Attack and decay
We model opening and closing of the "shutter", using linear curves
$$
\begin{equation}
I_{lin}(t) = t C,
\end{equation}
$$
and exponential curves
$$
\begin{equation}
I_{exp}(t) = e^{\lambda t},
\end{equation}
$$
where $t$ is time and $C$ and $\lambda$ are constants. If we compute the time derivatives we obtain
$$
\begin{equation}
\frac{dI_{lin}(t)}{dt} = C,
\end{equation}
$$
and
$$
\begin{equation}
\frac{dI_{exp}(t)}{dt} = \lambda e^{\lambda t},
\end{equation}
$$
Which means for linear curves we simply add some constant, and for exponential curves we multiply with some constant. This can be applied to rbg values every step for example to modify intensity based on some constant value. Discretizing for some color channel $c with subscript and superscript denoting time and space respectively, we obtain
$$
\begin{equation}
c^{t+1}_i = c^t_i + C,
\end{equation}
$$
for the linear case and
$$
\begin{equation}
c^{t+1}_i = \lambda c^t_i,
\end{equation}
$$
for the exponential

Note that our eyes are not perceiving intensity linearly, and the led's pixel value to perceived intensity is also not linear. So trial and error is key to find something that looks nice. 

### Modelling flow and diffusion
To make our LEDs come alive, we introduce 2 physical concepts; diffusion and flow. Physically, diffusion is when change in time is proportional to change in space squared. This results in peaks spreading out as they fade, creating a more natural dissipation of energy. Mathematically it's given by
$$
\begin{equation}
\frac{dI(t)}{dt} = \nabla^2I,
\end{equation}
$$
where again, $I$ is intensity here. But, could be any value. Now, numerically, the spacial gradient is just the difference between neighbouring leds, computed on the whole array. Conceptually this corresponds to a moving average of sorts.

A very simple implementation is
$$
\begin{equation}
c^{t+1}_i = 
\end{equation}
$$