(function(cb, main) {
    /* load templates */
    var xhr = new XMLHttpRequest();
    xhr.open('get', '/js_tpls');
    xhr.onreadystatechange = function() {
        if (xhr.readyState == 4) {
            cb(xhr.responseText, main);
        }
    };
    xhr.send(null);
})(function(strs, main) {
    /* basic templating system. Writing HTML in templates is easier than
     * adding it in html.h and compile everything again */

    var tpls = {},
        re_first_nl = /\n([\s\S]+)/,
        re_tpl_var = /%\{(\w+)\}/g, dz;

    strs = strs.split(/\n+--\n+/);

    for (var i=0, l=strs.length, parts; i<l; i++) {
        parts = strs[i].split(re_first_nl);
        tpls[parts[0]] = parts[1];
    }

    /* tpl('<p>%{name}</p>', {name: 'foo'}) -> '<p>foo</p>' */
    window.tpl = function(name, params) {
        if (arguments.length == 0) {
            return tpls;
        }
        if (!tpls.hasOwnProperty(name)) {
            return '';
        }

        if (!params) { return tpls[name]; }

        return tpls[name].replace(re_tpl_var, function(_, v) {
            return params.hasOwnProperty(v) ? params[v] : '';
        });
    };

    /* Basic DOM manipulation function */
    $obj = function ( el ) {
        if (''+el === el) {
            // html
            var div = document.createElement('div');
            div.innerHTML = el;
            el = div.firstChild;
        }
        this.el = el;
        this.wrapped = true;
    }
    window.$ = function ( el ) {
        if (el.wrapped) { return el; }
        return new $obj(el);
    }
    $obj.prototype.addTo = function( other ) {
        $(other).el.appendChild(this.el);
        return this;
    }

    dz = window.dz || {
        noop: function(){}
    };

    main(document.getElementsByTagName('body')[0], dz);
},
function(body, dz) {
    /************************
     * Main Dazibao JS Code *
     ************************/

    /* Note: the code here is not meant to be perfect, we're not doing a Web
     * project so we'll avoid spending too much time in JS code. */

    /** API Calls **/
    dz.api = {
        call: function(method, path, data, callback) {
            var xhr = new XMLHttpRequest();
            xhr.open(method, path);
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4) {
                    callback(xhr);
                }
            };
            xhr.send(data);
        },
        del: function(tlv_el) {
            var off = +tlv_el.getAttribute('data-offset');
            if (isNaN(off)) { return; }
            dz.api.call('post', '/tlv/delete/'+off, null, function(xhr) {
                if (xhr.status == 204) {
                    tlv_el.parentElement.removeChild(tlv_el);
                } else {
                    console.log("Error while deleting TLV at offset "+off, xhr);
                }
            });
        },
        compact: function(callback) {
            dz.api.call('post', '/compact', null, function(xhr) {
                callback(xhr.status == 200 ? +xhr.responseText : -1, xhr);
            });
        },
        hash: function(callback) {
            dz.api.call('get', '/hash', null, function(xhr) {
                callback(xhr.status == 200 ? +xhr.responseText : -1, xhr);
            });
        },
        addText: function(text, callback) {
            dz.api.call('post', '/tlv/add/text', text, function(xhr) {
                callback(xhr.status == 204);
            })
        },
        addTLV: function(data, callback) {
            dz.api.call('post', '/tlv/add/form', data, function(xhr) {
                callback(xhr);
            });
        }
    };


    /** 'Delete' Buttons **/
    var tlvs = document.getElementsByClassName('tlv'),
        actions = '<ul class="actions"><li class="deleteTLV">Delete</li></ul>';

    for (var i=0, l=tlvs.length; i<l; i++) {
        tlvs[i].innerHTML += actions;
    }

    body.addEventListener('click', function(e) {
        var el = e.target, tlv, off;
        if (el.className == 'deleteTLV') {
            tlv = el.parentElement.parentElement;
            dz.api.del(tlv);
        }
    }, false);

    /** Quotes in Text TLVs **/
    var quote_re = /(.*?)\s*--\s+((?:[^-\.]|-[^-])+)$/;

    for (var i=0, l=tlvs.length; i<l; i++) {
        if (+tlvs[i].getAttribute('data-type') != 2) {
            continue;
        }
        var b = tlvs[i].getElementsByTagName('blockquote')[0],
            res = quote_re.exec(b.innerHTML);

        if (res && res.length == 3) {
            b.innerHTML = tpl('quote', {
                text: res[1],
                author: res[2]
            });
        }
    }

    /** Notifications **/
    !function() {

        var min_timeout = 2000,
            max_timeout = 30000,
            timeout = 10000; // 10secs

        dz.prev_hash = 0;

        dz.enableDesktopNotifications = dz.noop;
        dz.notifications = !!window.Notification;

        if (dz.notifications) {
            if (Notification.permission == 'denied') {
                dz.notifications = false;
            } else if (Notification.permission != 'granted') {
                dz.enableDesktopNotifications = function() {
                    console.log('requesting permission for notifications...');
                    Notification.requestPermission(function (perm) {
                        if (!('permission' in Notification)) {
                            Notification.permission = perm;
                        }
                        dz.notifications =
                            (Notification.permission == 'granted');
                    });
                }
            }
        }

        dz.notify = function notify(title, text) {
            var opts = { icon: '/favicon.ico' };

            if (!dz.notifications) {
                return alert(title + text ? (': ' + text) : '');
            }

            if (text) {
                opts.body = text;
            }

            return new Notification('[DaziWeb] ' + title, opts);
        }

        function check_hash() {
            if (window.hash_check === false) { return; }

            dz.api.hash(function( hash ) {
                var changed = false;

                if (dz.prev_hash != 0) {
                    changed = hash > -1 && (hash != dz.prev_hash);
                }
                dz.prev_hash = hash;

                if (changed) {
                    timeout = Math.max(min_timeout, timeout/2);
                    dz.notify('Change detected!',
                           'A change on the current dazibao has been'
                          +' detected. You may want to reload the page to'
                          +' see it.');
                    console.log("Dazibao changed. Checking again in "
                                + timeout);
                } else {
                    timeout = Math.min(max_timeout, timeout*1.5);
                    console.log("Dazibao didn't change. Checking again in "
                                + timeout);
                }

                setTimeout(check_hash, timeout);
            });
        }

        check_hash();
    }();

    /** Dazibao settings **/
    !function() {
        var html = tpl('settings'), actions, buttons = {};

        document.body.innerHTML += html;
        actions = document.querySelector('#settings .actions');

        function addButton( text, id, callback ) {
            var b = document.createElement('li');
            id = '_' + id;
            b.className = id;
            buttons[id] = callback;
            b.innerText = b.textContent = text;
            actions.appendChild(b);
        }

        actions.addEventListener('click', function( e ) {
            var id = e.target.className;
            if (buttons[id]) {
                buttons[id](e.target);
            }
        }, false);

        /* -- notifications -- */
        if (dz.notifications) {
            addButton('Enable desktop notifications', 'edn', function( b ) {
                dz.enableDesktopNotifications();
                b.parentElement.removeChild(b);
            });
        }

        /* -- compacting -- */
        addButton('Compact', 'cpct', function() {
            dz.api.compact(function( saved, xhr ) {
                if (saved < 0) {
                    dz.notify("An error occured while compacting a Dazibao");
                    console.log(xhr);
                } else {
                    dz.notify("Saved " + saved + " bytes.");
                }
            });
        });

        /* -- adding a text TLV -- */
        addButton('Add a text', 'addtxttlv', function() {
            var text = prompt("Text?");

            dz.api.addText(text, function( ok ) {
                if (ok) {
                    dz.notify('text added, refresh the page to see it.');
                } else {
                    dz.notify('there was an error while adding your text.');
                }
            });
        });

        /* -- adding a TLV -- */
        var $modal = $(tpl('newtlv_modal')),
            form;
        $modal.addTo(body);

        document.getElementById('newtlv-cancel')
                    .addEventListener('click', function() {
            var inps = $modal.el.getElementsByTagName('input');
            $modal.el.className += ' hidden';
            for (var i=0, l=inps.length; i<l; i++) {
                if (inps[i].hasAttribute('name')) {
                    inps[i].value = '';
                }
            }
            
        }, false);

        form = document.getElementById('newtlv-form');
        form.addEventListener('submit', function( ev ) {

            var data = new FormData(form);

            ev.preventDefault();

            dz.api.addTLV(data, function(xhr) {
                dz.notify(xhr.status == 204
                          ? 'file added, refresh to see it'
                          : 'got an error while adding your file.');
                dz.prev_hash = 0;
                $modal.el.className += ' hidden';
            });

            return false;

        }, false);

        addButton('Add a file', 'addtlv', function() {
            $modal.el.className = $modal.el.className.replace(/\s*hidden/, '');
        });

    }();

});
