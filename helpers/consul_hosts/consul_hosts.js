function ConsulHosts( ) {

	var mods = {
		fs:			require( 'fs' ),
		http:		require( 'http' ),
		util:		require( 'util' ),
		fetch:      require( 'fetcher' ),
		log:		require( 'logger' ),
		getopt:		require( 'node-getopt' ),
		xtra:       require( 'extras' ),
	};

	var lgr = new mods.log( );
	var ftc = new mods.fetch( lgr );

	var conf = {
		port:		18500,
		chost:		'127.0.0.1',
		cport:		8500,
		cssl:		false,
		interval:	30000,
		spread:     2000,
		services:	[ ],
		fn:			null,
	};

	var server = null;
	var data = {};
	

	var sendResponse = function( res, code, data, type ) {

		var tm   = (new Date( )).toUTCString( );
		var out  = '';
		var hdrs = {
			'Server':		'Consul-Check',
			'Connection':	'close',
			'Date':			tm,
		};

		if( data ) {
			switch( typeof data ) {
				case 'string':
					out = data;
					break;
				case 'object':
					try {
						out  = JSON.stringify( data );
						type = 'text/json';
					} catch( err ) {
						out  = mods.util.inspect( data, true, 4 );
					}
					break;
			}

			hdrs['Expires']        = tm;
			hdrs['Cache-Control']  = 'no-cache';
			hdrs['Content-Type']   = ( type || 'text/plain' ) + '; charset=utf-8';
			hdrs['Content-Length'] = out.length;
		}

		res.writeHead( code || 200, hdrs );

		if( out ) {
			res.write( out, 'utf-8' );
		}

		res.end( );
	};


	var handleRequest = function( req, res ) {

		var name = req.url.replace( /^\//, '' );

		if( !name ) {
			var out = ''
			        + '/            Print this help\r\n'
			        + '/:name       Get results for consul service :name\r\n';

			sendResponse( res, 200, out );
			return;
		}

		if( !( name in data ) ) {
			sendResponse( res, 404, '' );
			return;
		}

		sendResponse( res, 200, data[name].results );
	};



	var fetchService = function( d, l ) {

		l.debug( 'Fetching nodes.' );

		conf.fn( d.opts, function( o, r, obj ) {

			var one, tmp = [ ];

			if( typeof obj !== 'object' ) {
				l.err( 'Failed to get JSON response.' );
				return;
			}

			if( !Array.isArray( obj ) ) {
				lgr.err( 'Response from consul was not an array.' );
				return;
			}

			if( obj.length !== d.prev ) {
				l.notice( 'Saw ' + obj.length + ' nodes.' );
				d.prev = obj.length;
			}

			for( var i = 0; i < obj.length; i++ ) {
				one = obj[i];

				if( !one.Node || !one.Address || !one.ServicePort ) {
					l.err( 'Response from consul looks invalid.' );
					return;
				}

				tmp.push( [ one.Node, one.Address, one.ServicePort ].join( ':' ) + '\n' );
			}

			d.results = tmp.join( '' );
		});
	};

	var startService = function( name ) {

		var l = lgr.delegate( 'Service:' + name );

		data[name] = {
			opts:		{
				host:		conf.chost,
				port:		conf.cport,
				path:		'/v1/catalog/service/' + name,
			},
			results:	[ ],
			prev:		0,
		};

		// spread the out a little bit, avoid bombing consul
		setTimeout( function( ) {

				setInterval( function( ) {
					fetchService( data[name], l );
				}, conf.interval );

				l.info( 'Started watching.' );
				fetchService( data[name], l );

			}, Math.random( ) * conf.spread );
	};


	var startWatching = function( ) {

		if( Array.isArray( conf.services ) ) {
			for( var i = 0; i < conf.services.length; i++ ) {
				startService( conf.services[i] );
			}
		} else {
			if( conf.services === 'all' ) {

				var opts = {
					host:	conf.chost,
					port:	conf.cport,
					path:	'/v1/catalog/services',
				}

				conf.fn( opts, function( o, r, obj ) {
					conf.services = Object.keys( obj );
					setTimeout( startWatching, 0 );

					// slow us down a bit
					var k = conf.services.length;
					while( k > 2 ) {
						conf.spread += 1000;
						k /= 2;
					}
				});

			} else {
				lgr.die( 'Cannot deal with services "' + conf.services + '".' );
			}
		}
	};


	var getOpt = function( obj, name ) {

		if( name in obj ) {
			return obj[name];
		}

		var envname = 'CONSUL_HOSTS_' + name.toUpperCase( );

		if( envname in process.env ) {
			return process.env[envname];
		}

		return null;
	};


	this.configure = function( path ) {

		var cfg, txt, args, opts, o;

		args = [
			[ 'h', 'help',       'Print this help.' ],
			[ 'c', 'config=ARG', 'Specify config file.' ],
			[ 's', 'consul=ARG', 'Specify consul source (host:port).' ],
			[ 't', 'tls',        'Consul uses TLS.' ],
		].concat( lgr.loggingArgs( ) );

		opts = mods.getopt.create( args ).bindHelp( ).parseSystem( ).options;
		lgr.loggingArgs( opts );

		if( ( o = getOpt( opts, 'config' ) ) ) {
			path = o;
		}

		mods.xtra.getJson( );

		if( !( cfg = JSON.readFile( path, lgr, mods.fs ) ) ) {
			lgr.die( 'Could not read file ' + path + ': ' + e.toString( ) );
		}

		// bring in whatever it says
		for( var k in cfg ) {
			conf[k] = cfg[k];
		}

		// allow overrides from opts or env
		if( ( o = getOpt( opts, 'tls' ) ) ) {
			conf.cssl = true;
		}
		if( ( o = getOpt( opts, 'consul' ) ) ) {
			var a = o.split( ':' );
			if( a.length < 2 ) {
				lgr.die( 'Invalid consul source option: ' + o );
			}
			conf.chost = a[0];
			conf.cport = a[1];
		}

		// is consul on ssl?
		if( conf.cssl ) {
			conf.fn = ftc.getSsl;
			lgr.notice( 'Fetching from consul over TLS.' );
		} else {
			conf.fn = ftc.get;
		}

		ftc.setGuessJson( true );
		ftc.allowUnauthorized( true );
	};

	this.run = function( ) {
		server = mods.http.createServer( handleRequest );
		server.listen( conf.port, function( ) {
			lgr.info( 'Server started on port ' + conf.port );
		});
		startWatching( );
	};
}

var ch = new ConsulHosts( );
ch.configure( './local.json' );
ch.run( );
