(function(body) {
    var api = {
        call: function(method, path, callback) {
            var xhr = new XMLHttpRequest();
            xhr.open(method, path);
            xhr.onreadystatechange = function() {
                if (xhr.readyState == 4) {
                    callback(xhr);
                }
            };
            xhr.send(null);
        },
        del: function(tlv_el) {
            var off = +tlv_el.getAttribute('data-offset');
            if (isNaN(off)) { return; }
            api.call('post', '/tlv/delete/'+off, function(xhr) {
                if (xhr.status == 204) {
                    tlv_el.parentElement.removeChild(tlv_el);
                } else {
                    console.log("Error while deleting TLV at offset "+off, xhr);
                }
            });
        }
    };


    /* 'delete' button */
    var tlvs = document.getElementsByClassName('tlv'),
        actions = '<ul class="actions"><li class="delete">Delete</li></ul>';

    for (var i=0, l=tlvs.length; i<l; i++) {
        tlvs[i].innerHTML += actions;
    }

    body.addEventListener('click', function(e) {
        var el = e.target, tlv, off;
        console.log(_e = el);
        if (el.className == 'delete') {
            tlv = el.parentElement.parentElement;
            api.del(tlv);
        }
    }, false);


})(document.getElementsByTagName('body')[0]);
