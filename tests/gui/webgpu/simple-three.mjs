
   import "./3d-shell/document.mjs"

	const canvas = document.createElement( "canvas", { width: 1920, height:1080 } );

	const {ctx,device} = await canvas.configured;

       
        import * as THREE from 'three/webgpu';

        // 1. Create the Scene (The 3D world container)
        const scene = new THREE.Scene();

        // 2. Create the Camera (Perspective view simulates human vision)
        // Parameters: Field of View (75 deg), Aspect Ratio, Near plane, Far plane
        const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
        camera.position.z = 5; // Move camera back so the cube is visible

        // 3. Create the Renderer (Renders the math onto the screen canvas)
	     const renderer = new THREE.WebGPURenderer({ domElement: canvas, context: ctx, device: device, antialias: true });
	     await renderer.init( {device, context:ctx} ).then( ()=>{
	         //renderer.setSize( window.innerWidth, window.innerHeight );
	         renderer.setClearColor( 0, 0.1 );
	         //renderer.setClearAlpha( 0 );
	     } );

		//console.log( "renderer?", renderer );
        renderer.setSize(window.innerWidth, window.innerHeight);
        document.body.appendChild(renderer.domElement);

        // 4. Create the Cube components (Geometry + Material = Mesh)
        const geometry = new THREE.BoxGeometry(1, 1, 1); // Width, Height, Depth
        const material = new THREE.MeshBasicMaterial({ color: 0x00ff00, wireframe: false }); 
        const cube = new THREE.Mesh(geometry, material);
        
        // Add the finished cube into our 3D world
        scene.add(cube);

        // 5. Animation Loop (Re-renders every frame to show movement)
        function animate() {
            requestAnimationFrame(animate);

            // Increment rotation angles on every frame refresh
            cube.rotation.x += 0.01;
            cube.rotation.y += 0.01;

            // Render the updated scene snapshot from the camera point of view
            renderer.render(scene, camera);
        }

	animate();
	canvas.show();