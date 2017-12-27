var dict = {
	first: [
		'apache',
		'nginx',
		'tomcat',
		'varnish',
		'mongo',
		'redis',
		'memcached',
		'mysql',
		'couchbase',
		'hornetq',
		'rabbitmq',
		'sendmail',
		'syslog',
	],
	second: [
		'uptime',
		'memory',
		'connections',
		'errors',
		'times',
		'locks',
		'requests',
	],
	third: [
		'webhost',
		'dbhost',
		'apphost',
	],
};


var net  = require( 'net' );
var path = new net.Socket( );
var stat = new net.Socket( );

function sendSet( sk, base, rval ) {

	var x, y, z, i, k = 0;

	for( x = 0; x < dict.first.length; x++ ) {
		for( y = 0; y < dict.second.length; y++ ) {
			for( z = 0; z < dict.third.length; z++ ) {
				for( i = 10; i < 99; i++, k++ ) {
					sk.write( dict.first[x] + '.' + dict.second[y] + '.' + dict.third[z] + i + ' ' + k + '\n' );
				}
			}
		}
	}

    var next = base + ( rval * Math.random( ) );
    setTimeout( sendSet, next, sk, base, rval );
}



path.connect( 9225, '127.0.0.1', function( ) {
    sendSet( path, 9900, 200 );
});

stat.connect( 9125, '127.0.0.1', function( ) {
    sendSet( stat, 100, 500 );
});

