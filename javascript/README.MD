# Iris Codec Javascript API
[![NPM Version](https://img.shields.io/npm/v/iris-codec?style=for-the-badge&logo=npm)](https://www.npmjs.com/package/iris-codec)
[![jsDelivr](https://img.shields.io/jsdelivr/npm/hm/iris-codec?style=for-the-badge&color=orange&logo=jsdelivr)](https://www.jsdelivr.com/package/npm/iris-codec)
[![JavaScript CI](https://img.shields.io/github/actions/workflow/status/IrisDigitalPathology/Iris-Codec/emcmake-wasm-CI.yml?style=for-the-badge&logo=javascript&label=JavaScript%20CI)](https://github.com/IrisDigitalPathology/Iris-Codec/actions/workflows/javascript-CI.yml)

Copyright &copy; 2025 Iris Developers; MIT Software License

## ⚠️ **Important**

**Iris RESTful Server is STRONGLY recommended for production use.** This WebAssembly module comes with severe performance penalties and should only be used when server deployment is not possible.

**Performance Comparison:**

1. **[Iris RESTful Server](https://github.com/IrisDigitalPathology/Iris-RESTful-Server)** (RECOMMENDED): 
   - **5000 tiles/sec** tile serving performance on Raspberry Pi
   - Server-side processing with virtual memory mapping
   - Easy integration with [OpenSeaDragon IrisTileSource](https://github.com/openseadragon/openseadragon/blob/master/src/iristilesource.js)

2. **[Iris-Codec WebAssembly](https://www.npmjs.com/package/iris-codec)** (USE ONLY WHEN NECESSARY):
   - **< 50% of RESTful Server tile performance** (in most circumstances)
   - Expect 1+ second delays per slide for initial metadata processing vs milliseconds with RESTful Server due to client-side file validation
   - Major advantage: Works with static file hosting (S3, GCS) that support streaming
   - **IMPORTANT: Load all slides in a case simultaneously** to amortize the 1+ second metadata overhead (only consts 1-2 MB memory per slide of RAM) 

## Installation

### Iris RESTful Server (Recommended)
For production deployments, use [Iris RESTful Server](https://github.com/IrisDigitalPathology/Iris-RESTful-Server). Deploy using [official container images](https://ghcr.io/irisdigitalpathology/iris-restful:latest).

### WebAssembly Module

**CDN (Recommended):**
```html
<script type="module">
import createModule from 'https://cdn.jsdelivr.net/npm/iris-codec@latest/iris-codec.js';

const irisCodec = await createModule();
console.log("Iris-Codec loaded");
</script>
```

**NPM (Node.js/Testing):**
```sh
npm i iris-codec
```

## ⚠️ **CRITICAL PERFORMANCE WARNING**

**NEVER use this WebAssembly package for server deployment!** If you use this as an intermediate layer between S3 and clients (as part of a NodeJS server implememtation for example), this WASM module will **severely bottleneck** your application. This is **ONLY** a client side module.

**Use Iris RESTful Server instead if you are depolying a slide server.** This WebAssembly module is only for scenarios where you cannot deploy a server and must use static file hosting from an S3 instance (AWS/GCS/MinIO).


## API Usage

### Iris RESTful API (Recommended)
Simple HTTP endpoints for slide data:
```
GET <URL>/slides/<slide-name>/metadata
GET <URL>/slides/<slide-name>/layers/<layer>/tiles/<tile>
```

**WADO-RS Support:**
```
GET <URL>/studies/<study>/series/<UID>/metadata
GET <URL>/studies/<study>/series/<UID>/instances/<layer>/metadata
GET <URL>/studies/<study>/series/<UID>/instances/<layer>/frames/<tile>
```

See [Iris RESTful Server documentation](https://github.com/IrisDigitalPathology/Iris-RESTful-Server) for setup.

### WebAssembly API

⚠️ **Performance Note**: Initial slide opening incurs significant overhead (~2+ seconds) due to client-side metadata validation. Consider this when designing your user experience.

The API uses Promise-based methods for asynchronous operations. Metadata is returned as IrisCodec::Abstraction C++ types exposed via Emscripten bindings (see included TypeScript definitions). Image tiles are returned as Blob objects.

**Basic Setup:**
```js
(async () => {
  const irisCodec = await createModule();
  const url = "https://example.com/slide.iris";
  
  // Validate file structure (optional but recommended)
  // This performs deep validation of the internal offset chain
  const result = await irisCodec.validateFileStructure(url);
  if (result.flag !== irisCodec.ResultFlag.IRIS_SUCCESS) {
    throw new Error(`Validation failed: ${result.message}`);
  }
  
  // Open slide
  const slide = await irisCodec.openIrisSlide(url);
  if (!slide) throw new Error("Failed to open slide");
  
  // Get slide info
  const info = slide.getSlideInfo();
  console.log(`${info.extent.width}x${info.extent.height}px`);
  
  // Get a tile
  const tileBlob = await slide.getSlideTile(0, 0); // layer 0, tile 0
  
  // Clean up
  slide.delete();
})();
```

**Get Detailed Slide Information:**
```js
(async () => {
  const irisCodec = await createModule();
  const slide = await irisCodec.openIrisSlide("https://example.com/slide.iris");
  
  const info = slide.getSlideInfo();
  
  // Basic slide information
  console.log("=== SLIDE METADATA ===");
  console.log(`Dimensions: ${info.extent.width}x${info.extent.height}px`);
  console.log(`Total Layers: ${info.extent.layers.size()}`);
  console.log(`Format: ${info.format.value}`);
  console.log(`Encoding: ${info.encoding.value}`);
  
  // Layer details
  console.log("\n=== LAYER INFORMATION ===");
  for (let i = 0; i < info.extent.layers.size(); i++) {
    const layer = info.extent.layers.get(i);
    console.log(`Layer ${i}:`);
    console.log(`  - Scale: ${layer.scale}x`);
    console.log(`  - Downsample: ${layer.downsample}`);
    console.log(`  - Tiles: ${layer.xTiles}x${layer.yTiles} (${layer.xTiles * layer.yTiles} total)`);
  }
  
  // Metadata information
  console.log("\n=== METADATA ===");
  console.log(`Codec Version: ${info.metadata.codec.major}.${info.metadata.codec.minor}.${info.metadata.codec.build}`);
  console.log(`Microns Per Pixel: ${info.metadata.micronsPerPixel}`);
  console.log(`Magnification: ${info.metadata.magnification}`);
  console.log(`Associated Images: ${info.metadata.associatedImages.keys()}`);
  console.log(`Annotations: ${info.metadata.annotations.keys()}`);
  
  // Or if you are more comfortable with JSON...
  // Get all metadata as JSON in one go
  console.log("\n=== FULL METADATA JSON ===");
  const metadataJSON = slide.getMetadataJSON();
  console.log(metadataJSON);
  
  slide.delete();
})();
```

**Display a Single Tile:**
```html
<img id="tileImage" width="128" height="128" alt="Loading..."/>
<script type="module">
import createModule from 'https://cdn.jsdelivr.net/npm/iris-codec@latest/iris-codec.js';

(async () => {
  try {
    const irisCodec = await createModule();
    const slide = await irisCodec.openIrisSlide("https://example.com/slide.iris");
    const tileBlob = await slide.getSlideTile(0, 0);
    
    const objectUrl = URL.createObjectURL(tileBlob);
    document.getElementById("tileImage").src = objectUrl;
    
    // Clean up
    document.getElementById("tileImage").onload = () => {
      URL.revokeObjectURL(objectUrl);
      slide.delete();
    };
  } catch (error) {
    console.error(error);
  }
})();
</script>
```

**Complete Tile Grid Example:**

This example displays all tiles in a grid layout with performance timing:

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Iris-Codec WASM: Tile Grid</title>

  <style>
    /* The grid container: auto-flows row by row */
    #tileGrid {
      display: grid;
      justify-content: start;      /* left-align if container is wide */
      align-content: start;        /* top-align if container is tall */
    }
    #tileGrid img {
      width: 128px;
      height: 128px;
      object-fit: cover;           /* crop or letterbox if needed */
      background: #f0f0f0;         /* placeholder color */
    }
  </style>
</head>

<body>
  <h1>Iris-Codec WASM: Tile Grid</h1>
  <div id="loadSlideText">Slide validation/load time: —</div>
  <div id="loadTilesText">Load time: —</div>
  <div id="tileGrid"></div>

  <script type="module">
  import createModule from 'https://cdn.jsdelivr.net/npm/iris-codec@latest/iris-codec.js';
  
  // ————— Main Entry Point —————
  (async () => {
    const irisCodec = await createModule();
    const url = "https://irisdigitalpathology.s3.us-east-2.amazonaws.com/example-slides/cervix_4x_jpeg.iris";
    try {
      // Time slide opening
      const slideStart = performance.now();
      
      // Validate file structure
      const validationResult = await irisCodec.validateFileStructure(url);
      if (validationResult.flag !== irisCodec.ResultFlag.IRIS_SUCCESS) {
        throw new Error(`File validation failed. ${validationResult.message}`);
      }
      
      const slide = await irisCodec.openIrisSlide(url);
      if (!slide) {
        throw new Error("Failed to open the slide file. Please check the file URL and try again.");
      }
      const slideEnd = performance.now();
      document.getElementById("loadSlideText").textContent = 
        `Slide validation/load time: ${Math.round(slideEnd - slideStart)} ms`;

      // 1) Read layer info
      const layer = 1;
      const info = slide.getSlideInfo();
      const extent = info.extent.layers.get(layer);
      const { xTiles, yTiles } = extent;
      const nTiles = xTiles * yTiles;

      // 2) Configure the grid container
      const gridEl = document.getElementById("tileGrid");
      gridEl.style.gridTemplateColumns = `repeat(${xTiles}, 128px)`;
      gridEl.style.gridTemplateRows = `repeat(${yTiles}, 128px)`;

      // 3) Create placeholder img elements first
      const imgElements = [];
      for (let i = 0; i < nTiles; i++) {
        const img = new Image(128, 128);
        img.style.backgroundColor = 'transparent';
        gridEl.appendChild(img);
        imgElements.push(img);
      }

      // 4) Fetch tiles and render as they arrive
      const tilesStart = performance.now();
      let tilesLoaded = 0;
      const objectUrls = [];
      
      const tilePromises = Array.from({ length: nTiles }, async (_, idx) => {
        try {
          const blob = await slide.getSlideTile(layer, idx);
          const url = URL.createObjectURL(blob);
          objectUrls.push(url);
          
          // Render immediately as tile arrives
          imgElements[idx].src = url;
          imgElements[idx].style.backgroundColor = 'transparent';
          
          tilesLoaded++;
          const elapsed = Math.round(performance.now() - tilesStart);
          document.getElementById("loadTilesText").textContent = 
            `${tilesLoaded}/${nTiles} tiles loaded in ${elapsed} ms`;
          
          return blob;
        } catch (error) {
          console.error(`Failed to load tile ${idx}:`, error);
          return null;
        }
      });

      // Wait for all tiles to complete
      await Promise.all(tilePromises);
      const tilesEnd = performance.now();
      document.getElementById("loadTilesText").textContent = 
        `All ${nTiles} tiles loaded in ${Math.round(tilesEnd - tilesStart)} ms`;

      // 5) Cleanup on unload
      window.addEventListener("beforeunload", () => {
        // Revoke all blob URLs
        objectUrls.forEach(u => URL.revokeObjectURL(u));
        // Free the slide
        slide.delete();
      });

    } catch (err) {
      console.error(err);
      alert("Error: " + err.message);
    }
  })();
  </script>
</body>
</html>
```
