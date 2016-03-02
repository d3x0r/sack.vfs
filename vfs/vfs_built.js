 if (typeof Object.summon !== 'function') {
     Object.summon = function (o) {
         var object = {};
         object.prototype = o;
         return object;
     };
 }
 if (typeof Object.develop !== 'function') {
     Object.develop = function (o) {
         function F() {}
         F.prototype = o;
         return new F();
     };
 }
