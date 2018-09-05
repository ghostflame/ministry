var http = require( 'http' );
var fs   = require( 'fs' );

var data = {
	str:		null,
	len:		0,
};


var logmsg = function( msg ) {
	console.log( '[' + ((new Date).getTime( ) / 1000).toFixed( 3 ) + '] ' + msg );
};

var respond = function( req, res ) {

	if( req.url != '/data' ) {
		logmsg( 'Unexpected path: ' + req.url );
		res.writeHead( 404 );
		res.end( );
		return;
	}

	logmsg( 'Serving data.' );
	res.writeHead( 200, { 'Content-Length': data.len } );
	res.write( data.str );
	res.end( );
};

if( process.argv.length < 3 ) {
	logmsg( 'No data file provided.' );
	process.exit( 1 );
}
data.str = fs.readFileSync( process.argv[2] );
data.len = data.str.length;

port = process.argv[3] || 10000;

var svr = http.createServer( respond );
svr.listen( port, function( ) {
	logmsg( `Server listening on port ${port}.` );
});

