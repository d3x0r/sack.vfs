

/*
				renderer = new THREE.WebGLRenderer( { antialias: true } );
				renderer.setPixelRatio( window.devicePixelRatio );
				renderer.setSize( window.innerWidth*4/5, window.innerHeight*4/5 );
				renderer.sortObjects = false;
				renderer.autoClear = false;
				renderer.shadowMap.enabled = true;
				renderer.gammaInput = true;
				renderer.gammaOutput = true;
*/



	this.render = function ( scene, camera, renderTarget, forceClear ) {

		if ( ! ( camera && camera.isCamera ) ) {

			console.error( 'THREE.WebGLRenderer.render: camera is not an instance of THREE.Camera.' );
			return;

		}

		if ( _isContextLost ) return;

		// reset caching for this frame

		_currentGeometryProgram = '';
		_currentMaterialId = - 1;
		_currentCamera = null;

		// update scene graph

		if ( scene.autoUpdate === true ) scene.updateMatrixWorld();

		// update camera matrices and frustum

		if ( camera.parent === null ) camera.updateMatrixWorld();

		if ( vr.enabled ) {

			camera = vr.getCamera( camera );

		}

		_projScreenMatrix.multiplyMatrices( camera.projectionMatrix, camera.matrixWorldInverse );
		_frustum.setFromMatrix( _projScreenMatrix );

		lightsArray.length = 0;
		shadowsArray.length = 0;

		spritesArray.length = 0;
		flaresArray.length = 0;

		_localClippingEnabled = this.localClippingEnabled;
		_clippingEnabled = _clipping.init( this.clippingPlanes, _localClippingEnabled, camera );

		currentRenderList = renderLists.get( scene, camera );
		currentRenderList.init();

		projectObject( scene, camera, _this.sortObjects );

		if ( _this.sortObjects === true ) {

			currentRenderList.sort();

		}

		//

		if ( _clippingEnabled ) _clipping.beginShadows();

		shadowMap.render( shadowsArray, scene, camera );

		lights.setup( lightsArray, shadowsArray, camera );

		if ( _clippingEnabled ) _clipping.endShadows();

		//

		_infoRender.frame ++;
		_infoRender.calls = 0;
		_infoRender.vertices = 0;
		_infoRender.faces = 0;
		_infoRender.points = 0;

		if ( renderTarget === undefined ) {

			renderTarget = null;

		}

		this.setRenderTarget( renderTarget );

		//

		background.render( currentRenderList, scene, camera, forceClear );

		// render scene

		var opaqueObjects = currentRenderList.opaque;
		var transparentObjects = currentRenderList.transparent;

		if ( scene.overrideMaterial ) {

			var overrideMaterial = scene.overrideMaterial;

			if ( opaqueObjects.length ) renderObjects( opaqueObjects, scene, camera, overrideMaterial );
			if ( transparentObjects.length ) renderObjects( transparentObjects, scene, camera, overrideMaterial );

		} else {

			// opaque pass (front-to-back order)

			if ( opaqueObjects.length ) renderObjects( opaqueObjects, scene, camera );

			// transparent pass (back-to-front order)

			if ( transparentObjects.length ) renderObjects( transparentObjects, scene, camera );

		}

		// custom renderers

		spriteRenderer.render( spritesArray, scene, camera );
		flareRenderer.render( flaresArray, scene, camera, _currentViewport );

		// Generate mipmap if we're using any kind of mipmap filtering

		if ( renderTarget ) {

			textures.updateRenderTargetMipmap( renderTarget );

		}

		// Ensure depth buffer writing is enabled so it can be cleared on next render

		state.buffers.depth.setTest( true );
		state.buffers.depth.setMask( true );
		state.buffers.color.setMask( true );

		state.setPolygonOffset( false );

		if ( vr.enabled ) {

			vr.submitFrame();

		}

		// _gl.finish();

	};

