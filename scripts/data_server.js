var http = require( 'http' );
var fs   = require( 'fs' );
var optM = require( 'node-getopt' );

var args = [
	[ 'h', 'help',              'Print this help.' ],
	[ 'p', 'port=i',            'Port to serve on.' ],
	[ 'd', 'data=ARG',          'File to read for stats data.' ],
	[ 'm', 'metrics=ARG',       'File to read for metrics data.' ],
];

var data = {
	'/data':		null,
	'/metrics':		null,
	'/':			'',
};

var port = 10000;


var logmsg = function( msg ) {
	console.log( '[' + ((new Date).getTime( ) / 1000).toFixed( 3 ) + '] ' + msg );
};

var respond = function( req, res ) {

	if( !( req.url in data ) ) {
		logmsg( 'Unexpected path: ' + req.url );
		res.writeHead( 404 );
		res.end( );
		return;
	}

	logmsg( 'Serving ' + req.url );
	res.writeHead( 200, { 'Content-Length': data[req.url].length } );
	res.write( data[req.url] );
	res.end( );
};


var opt = optM.create( args );
opt.setHelp(
	"Usage: node data_server.js --help\n" +
	"       node data_server.js [--port <port>] [--data <stats file>] [--metrics <metrics file>]\n\n" +
	"This small webserver servers static stats/metrics content for testing ministry fetch.\n\n" );

var opts = opt.bindHelp( ).parseSystem( ).options;

if( opts.metrics ) {
	data['/metrics'] = fs.readFileSync( opts.metrics );
	logmsg( `/metrics taken from ${opts.metrics}.` ); 
}
if( opts.data ) {
	data['/data'] = fs.readFileSync( opts.data );
	logmsg( `/data taken from ${opts.data}.` );
}
if( opts.port ) {
	port = parseInt( opts.port, 10 );
}

var str = '';
for( var p in data ) {
	if( data[p] ) {
		str += p + "\n";
	}
}
data['/'] = str;

var svr = http.createServer( respond );
svr.listen( port, function( ) {
	logmsg( `Server listening on port ${port}.` );
});

