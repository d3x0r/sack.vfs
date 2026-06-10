
import {Viewer,THREE} from "./3d-shell/three-js-view.mjs"
import {lnQuat} from "./3d-shell/lnQuatSq.mjs"

const viewer = new Viewer( tickScene );

// first object added is also the camera ... gets controls attached to it.
const	myMotion = Viewer.addModelToScene2( new THREE.Object3D() );
	myMotion.dipole = new lnQuat( 0, 0, 0, 1 ).update();
   myMotion.orientation.set( 0, 0, -Math.PI/2, 0 );
	myMotion.position.set( 3, 0, 0 );


        const geometry = new THREE.BoxGeometry(1, 1, 1); // Width, Height, Depth
        const material = new THREE.MeshBasicMaterial({ color: 0x00ff00, wireframe: false }); 
        const cube = new THREE.Mesh(geometry, material);
        
        // Add the finished cube into our 3D world
        Viewer.addModelToScene2(cube);

function tickScene( tick ) {
	            cube.rotation.x += 0.01;
            cube.rotation.y += 0.01;
	// this should update objects.
}