## FFT in Webassembly 

Original project here,
https://github.com/AWSM-WASM/PulseFFT

A WebAssembly implementation of kissFFT, the 'keep-it-simple-stupid' Fast Fourier Transform library written in C. This project allows forward and inverse FFTs to be computed with low-level processes in the browser with the performant WebAssembly format.


## Get Started

### Download Emscripten

```
$ git clone https://github.com/juj/emsdk.git
$ cd emsdk
$ ./emsdk install --build=Release sdk-incoming-64bit binaryen-master-64bit
$ ./emsdk activate --build=Release sdk-incoming-64bit binaryen-master-64bit
```

# Install emscripten build environment

```
source emsdk_env.sh --build=Release
emsdk list
```

# Test hello example
```
emcc hello.c -s WASM=1 -o hello.html
emrun --no_browser --port 8080
```

## build

```
source emsdk_env.sh --build=Release
make -f Makefile.emscripten
```

## Usage

This library is a WebAssembly implementation of kissFFT; consult the [kissFFT README](https://github.com/bazaar-projects/kissfft) if you want more info on the internal FFT details. 

### Instantiate Pulse

```
let pulse = {};
loadPulse()
    .then(module => {
        pulse = module;
    })
```
### Real Input
```
// Create the WebAssembly instance.
const fft = new pulse.fftReal(size);
fft.forward(input)

```
### Complex Input
```
// Create the WebAssembly instance.
const fft = new pulse.fftComplex(size);
fft.forward(input)
```



## License

licensed under the MIT License.  
KissFFT is licensed under the Revised BSD License.
