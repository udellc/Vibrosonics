- The dependencies directory contains libary dependencies that are needed to compile the code. "arduinoFFT_FLOAT is the same as the original "arduinoFFT" library, except that it uses floating point precision instead of doubles to increase processing speed. Copy the contents of this directory to your Arduino libraries folder, typically located under:
	 "C:\Users\{USERNAME}\Documents\Arduino\libraries" 

	- NOTE: "Mozzi-master" library only needs to be copied if using Vibrosonics-Mozzi. In other words, this library is not needed for Vibrosonics-Interrupt.

- Vibrosonics-Mozzi uses the Mozzi library for audio synthesis and serves as a proof of concept. Though this version works as a proof of concept, it is very limited and complicated. Recommend to use Vibrosonics-Interrupt for further development and disregard the Mozzi version. 

- Vibrosonics-Interrupt uses a timer interrupt to asynchronously sample and output audio which significantly improves performance and reduces code complexity.