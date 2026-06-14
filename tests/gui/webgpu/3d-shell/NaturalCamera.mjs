/**
 * @author d3x0r / https://github.com/d3x0r
 */

import * as THREE from "three/webgpu"

// lnQuat is used in Motion, but not directly here
//import {lnQuat} from "../3d/src/lnQuatSq.js"
import {THREE_consts,Motion} from "./personalFill.mjs"


// object is a three.JS object which will updated with user inputs.

const Accel1 = 2*Math.PI / 24;
const linAccel1 = 10;

// these could be overridden
//  
export const keys = { LEFT: 37, UP: 38, RIGHT: 39, BOTTOM: 40
        , A:65, S:83, D:68, W:87, SPACE:32, C:67, E:69, Q:81, SHIFT:16 
        , Z:90
        , X:88
        , TAB: 9
        , NUM0: 45
        , NUM1: 35
        , NUM2: 40
        , NUM3: 34
        , NUM4: 37
        , NUM4a: 100
        , NUM5: 12
        , NUM6: 39
        , NUM7: 36
        , NUM8: 38
        , NUM9: 33
        , NUMDOT: 46
        , ENTER:13
        , TILDE:192
	};


export function NaturalCamera( object, domElement ) {

	var self = this;
	this.object = object;
	this.motion = new Motion(object);

	this.domElement = ( domElement !== undefined ) ? domElement : document;

	this.enabled = false;



	// internals
	this.moveSpeed = 10 * 12 * 0.0254;
	this.fastMove = false;
	const scope = this;
	
	// 2d scaled screen point - prior position
	this.rotateStart = new THREE.Vector2();
	// 2d scaled screen point - current
	this.rotateEnd = new THREE.Vector2();
	// temp for rotation difference of start and end
	this.rotateDelta = new THREE.Vector2();

	let phiDelta = 0;
	let thetaDelta = 0;

	this.userRotate = false;
	this.alignUp = true;

	this.rotateLeft = function ( angle ) {
		if ( angle === undefined )  angle = 0;//getAutoRotationAngle();
		thetaDelta -= angle;
	};

	this.rotateRight = function ( angle ) {
		if ( angle === undefined )  angle = 0;//getAutoRotationAngle();
		thetaDelta += angle;

	};

	this.rotateUp = function ( angle ) {
		if ( angle === undefined )  angle = 0;//getAutoRotationAngle();
		phiDelta += angle;

	};

	this.rotateDown = function ( angle ) {
		if ( angle === undefined )  angle = 0;//getAutoRotationAngle();
		phiDelta -= angle;
	};

	this.update = function ( tick ) {
	    scope.object.matrixWorldNeedsUpdate = true;
	    //scope.object.matrixNeedsUpdate = true;
		console.log( "update:", scope.userRotate, scope.motion.torque, scope.motion.acceleration );
		if( !scope.userRotate ) return;
		touchUpdate();
		//console.log( "userRotate is off for update??", scope.userRotate, phiDelta, thetaDelta );

		const roll = scope.alignUp?scope.motion.orientation.getRoll():0;
		if( phiDelta || thetaDelta || roll ){
			scope.motion.rotation.x = -phiDelta;
			scope.motion.rotation.y = thetaDelta;

			// always face 'up'
			scope.motion.rotation.z = -roll/tick; // normalize rotation to full rotation for this tick.

			scope.motion.rotation.dirty = true;
			//scope.motion.rotation.yaw(  );
			thetaDelta = 0;
			phiDelta = 0;

		} else {
			scope.motion.rotation.x = 0;
			scope.motion.rotation.y = 0;
			scope.motion.rotation.z = 0;
			scope.motion.rotation.dirty = true;
		}
			scope.motion.move( scope.object, tick );			
		console.log( "So we move the motion..." );
	};

	

	function onMouseDown( event ) {
	         //console.log( "down" );
		if ( scope.enabled === false ) return;
		if( !scope.userRotate ) return;

		event.preventDefault();

		//scope.rotateStart.set( event.clientX, event.clientY );


	}

	function onMouseMove( event ) {
		if ( scope.enabled === false ) return;

		event.preventDefault();


	//if( event.movementX
		scope.rotateDelta.set( event.movementX, event.movementY );

	        scope.rotateDelta.x = 25 * (scope.rotateDelta.x / window.innerWidth)
        	scope.rotateDelta.y = 25 * (scope.rotateDelta.y / window.innerHeight)

		thetaDelta -= ( 2 * Math.PI * scope.rotateDelta.x  );
		phiDelta += ( 2 * Math.PI * scope.rotateDelta.y );

	}

	function onMouseUp( event ) {

		if ( scope.enabled === false ) return;
		if ( scope.userRotate === false ) return;


	}

	function onMouseWheel( event ) {

		if ( scope.enabled === false ) return;
	}

	var keyEvent = null;

	function onKeyDown( event ) {
        	console.log( "key down:", event, scope.enabled, scope.userRotate );
		if ( scope.enabled === false ) return;

		if( !scope.userRotate ) {
                	moreKeys( event, scope.motion, true );
			if( keyEvent )
				keyEvent( event, true );
			return;
		}
		const myMotion = scope.motion;
		switch ( event.keyCode ) {
		default:
			console.log( "Should be sending standard event..." );
				moreKeys( event, scope.motion, true );
			if( keyEvent )
				keyEvent( event, true );
			break;
		case keys.SHIFT:
			if( !self.fastMove ) {
			self.fastMove = true;
			scope.motion.speed.multiplyScalar( 10 );
			}
			break;
            case keys.SPACE:
            case keys.E:
                scope.motion.speed.y = self.moveSpeed * ( self.fastMove?10:1);
                break;
            case keys.C:
            case keys.Q:
                scope.motion.speed.y = -self.moveSpeed * ( self.fastMove?10:1);
				break;
			case keys.A:
				scope.motion.speed.x = self.moveSpeed * ( self.fastMove?10:1);
				break;
			case keys.W:
				scope.motion.speed.z = -self.moveSpeed * ( self.fastMove?10:1);
				break;
			case keys.S:
				scope.motion.speed.z = self.moveSpeed * ( self.fastMove?10:1);
				break;
			case keys.D:
				scope.motion.speed.x = -self.moveSpeed * ( self.fastMove?10:1);
				break;
		}
		const s = scope.motion.speed;
		const l = s.x * s.x + s.y*s.y + s.z*s.z;
		s.θ = Math.sqrt( l );
		if( s.θ ) {
			s.nx = s.x/s.θ;
			s.ny = s.y/s.θ;
			s.nz = s.z/s.θ;
		}

	}

	function moreKeys( event, myMotion, isDown ) {
        	console.log( "more keys (numpad/mode change..." );
		switch( event.keyCode ) {
		case keys.TAB:
			event.preventDefault();
			if( isDown )   {
				//mode = 1-mode;
				switch(mode ) {
				case 1: // is locked, want unlock
					document.exitPointerLock();
					console.log( "unlocked" );
					scope.userRotate = false;
					mode = 0;
					break;
				case 0: // is unlocked, want lock.
					if( renderer.domElement ) {
						renderer.domElement.requestPointerLock(); 
					}
					mode = 1;
					console.log( "locked" );
					scope.userRotate = true;
					break;
				}
			}
			break;
		case keys.X:
			myMotion.stabilizeVelocity = isDown?0.9:0;
			break;
		case keys.Z:
			myMotion.stabilizeRotation = isDown?0.9:0;
			break;
		case keys.TILDE:
			if( isDown )   {
				scope.userRotate = !scope.userRotate;
			}
			break;
		case keys.NUM2:
			if( isDown )
				myMotion.torque.x = -Accel1;
			else
				myMotion.torque.x = 0;
			myMotion.torque.dirty = true;
			break;
		case keys.NUM8:
			if( isDown )
				myMotion.torque.x = Accel1;
			else
				myMotion.torque.x = 0;
			myMotion.torque.dirty = true;
			break;

		case keys.NUM4:
		case keys.NUM4a:
			if( isDown )
				myMotion.torque.y = Accel1;
			else
				myMotion.torque.y = 0;
			myMotion.torque.dirty = true;
                        console.log( "rotate!" );
			break;
		case keys.NUM6:
			if( isDown )
				myMotion.torque.y = -Accel1;
			else
				myMotion.torque.y = 0;
			myMotion.torque.dirty = true;
			break;

		case keys.NUM7:
			if( isDown )
				myMotion.torque.z = -Accel1;
			else
				myMotion.torque.z = 0;
			myMotion.torque.dirty = true;
			break;
		case keys.NUM9:
			if( isDown )
				myMotion.torque.z = +Accel1;
			else
				myMotion.torque.z = 0;
			myMotion.torque.dirty = true;
			break;


		case keys.NUM1:
			if( isDown )
				myMotion.acceleration.x = -linAccel1;
			else
				myMotion.acceleration.x = 0;
			break;
		case keys.NUM3:
			if( isDown )
				myMotion.acceleration.x = +linAccel1;
			else
				myMotion.acceleration.x = 0;
			break;

		case keys.NUM0:
			if( isDown )
				myMotion.acceleration.z = +linAccel1;
			else
				myMotion.acceleration.z = 0;
			break;

		case keys.NUMDOT:
			if( isDown )
				myMotion.acceleration.z = -linAccel1;
			else
				myMotion.acceleration.z = 0;
			break;

		case keys.NUM5:
			if( isDown )
				myMotion.acceleration.y = linAccel1;
			else
				myMotion.acceleration.y = 0;
			break;

		case keys.ENTER:
			if( isDown )
				myMotion.acceleration.y = -linAccel1;
			else
				myMotion.acceleration.y = 0;
			break;

		}		
	}

	function onKeyUp( event ) {

	if( !scope.userRotate ) {
               	moreKeys( event, scope.motion, false );
		if( keyEvent )
			keyEvent( event, false );
		return;
	}
        switch ( event.keyCode ) {
		default:
			moreKeys( event, scope.motion, false );
			if( keyEvent )
				keyEvent( event, false );
			break;
		case scope.keys.SHIFT:
			if( self.fastMove ) {
				self.fastMove = false;
				scope.motion.speed.multiplyScalar( 1/10 );
			}
			break;
		case keys.SPACE:
		case keys.E:
			scope.motion.speed.y = 0;
			break;
		case keys.C:
		case keys.Q:
			scope.motion.speed.y = 0;
			break;

		case keys.A:
			scope.motion.speed.x = 0;
			break;
		case keys.W:
			scope.motion.speed.z = 0;
			break;
		case keys.S:
			scope.motion.speed.z = 0;
			break;
		case keys.D:
			scope.motion.speed.x = 0;
			break;
		}
		//switch ( event.keyCode ) {

		//		break;
		//}

	}

var touches = [];
if( typeof TouchList !== "undefined" )
	TouchList.prototype.forEach = function(c){ for( var n = 0; n < this.length; n++ ) c(this[n]); }

function touchUpdate() {
  if( touches.length == 1 ){
    var t = touches[0];
    if( t.new )
    {
      scope.rotateStart.set( t.x, t.y );
      t.new = false;
    }
    else {
            scope.rotateEnd.set( t.x, t.y );
      		scope.rotateDelta.subVectors( scope.rotateEnd, scope.rotateStart );

            scope.rotateDelta.x = -2 * (scope.rotateDelta.x / window.innerWidth)
            scope.rotateDelta.y = - 2 * (scope.rotateDelta.y / window.innerHeight)
      		scope.rotateLeft( Math.PI/2 * rotateDelta.x   );
      		scope.rotateUp( Math.PI/2 * rotateDelta.y );
            //console.log( rotateDelta )
      		scope.rotateStart.copy( scope.rotateEnd );
    }
  }
}

function onTouchStart( e ) {
  e.preventDefault();
  e.changedTouches.forEach( (touch)=>{
    touches.push( {ID:touch.identifier,
      x : touch.clientX,
      y : touch.clientY,
      new : true
    })
  })
}

function onTouchMove( e ) {
  e.preventDefault();
  e.changedTouches.forEach( (touchChanged)=>{
    var touch = touches.find( (t)=> t.ID === touchChanged.identifier );
    if( touch ) {
      touch.x = touchChanged.clientX;
      touch.y = touchChanged.clientY;
    }
  })
}

function onTouchEnd( e ) {
  e.preventDefault();
  e.changedTouches.forEach( (touchChanged)=>{
    var touchIndex = touches.findIndex( (t)=> t.ID === touchChanged.identifier );
    if( touchIndex >= 0 )
       touches.splice( touchIndex, 1 )
  })
}

    function ignore(event) {
        event.preventDefault();
    }
    this.disable = function() {
		if( !scope.enabled )  return;
		scope.enabled = false;

    	scope.domElement.removeEventListener( 'contextmenu', ignore, false );
    	scope.domElement.removeEventListener( 'mousedown', onMouseDown, false );
    	scope.domElement.removeEventListener( 'mousewheel', onMouseWheel, false );
    	scope.domElement.removeEventListener( 'DOMMouseScroll', onMouseWheel, false ); // firefox
    	window.removeEventListener( 'keydown', onKeyDown, false );
    	window.removeEventListener( 'keyup', onKeyUp, false );
    }

    this.enable = function(cb) {
		//return;
		keyEvent = cb;
		if( scope.enabled )  return;
		scope.enabled = true;

    	scope.domElement.addEventListener( 'contextmenu', ignore, false );
    	scope.domElement.addEventListener( 'mousedown', onMouseDown, false );
		scope.domElement.addEventListener( 'mousemove', onMouseMove, false );
		scope.domElement.addEventListener( 'mouseup', onMouseUp, false );

    	scope.domElement.addEventListener( 'mousewheel', onMouseWheel, false );
      	scope.domElement.addEventListener( 'touchstart', onTouchStart, false );
      	scope.domElement.addEventListener( 'touchmove', onTouchMove, false );
      	scope.domElement.addEventListener( 'touchend', onTouchEnd, false );

    	scope.domElement.addEventListener( 'DOMMouseScroll', onMouseWheel, false ); // firefox
    	window.addEventListener( 'keydown', onKeyDown, false );
    	window.addEventListener( 'keyup', onKeyUp, false );
    }

    this.enable();

};


// extend Object with a default event dispatcher
NaturalCamera.prototype = Object.create( THREE.EventDispatcher.prototype );
