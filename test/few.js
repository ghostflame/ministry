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
//		'dbhost',
//		'apphost',
	],
};

var net  = require( 'net' );
var path = new net.Socket( );
var stat = new net.Socket( );

function sendSet( sk ) {

	var x, y, z, i, k = 0;

	for( x = 0; x < dict.first.length; x++ ) {
		for( y = 0; y < dict.second.length; y++ ) {
			for( z = 0; z < dict.third.length; z++, k++ ) {
				sk.write( dict.first[x] + '.' + dict.second[y] + '.' + dict.third[z] + ' ' + k + '\n' );
			}
		}
	}
}



path.connect( 9225, '127.0.0.1', function( ) {
	setInterval( sendSet, 3, path );
});

stat.connect( 9125, '127.0.0.1', function( ) {
	setInterval( sendSet, 2, stat );
});
