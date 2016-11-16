var dict = {
	first: [
		'apache',
	],
	second: [
		'locks',
		'requests',
	],
	third: [
        'time',
	],
};

var net  = require( 'net' );
//var path = new net.Socket( );
var stat = new net.Socket( );

var extra = parseInt( ( process.argv[2] || '0' ) );

function randVal( count ) {

    var value = 10 * Math.random( );

    for( var i = 0; i < count; i++ ) {
        value += 10 * Math.random( );
    }

    return value;
};


function sendSet( sk ) {

	var x, y, z, i, k = 0;

	for( x = 0; x < dict.first.length; x++ ) {
		for( y = 0; y < dict.second.length; y++ ) {
			for( z = 0; z < dict.third.length; z++ ) {
				sk.write( dict.first[x] + '.' + dict.second[y] + '.' + dict.third[z] + ' ' + randVal( extra ) + '\n' );
			}
		}
	}
}



//path.connect( 9225, '127.0.0.1', function( ) {
//	setInterval( sendSet, 3, path );
//});

stat.connect( 9125, '127.0.0.1', function( ) {
	setInterval( sendSet, 2, stat );
});
