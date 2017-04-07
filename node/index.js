/**
 * Created by helidium on 3/29/17.
 */
module.exports = (function() {
    "use strict";
    try {
        return require(`./mns_${process.platform}_${process.versions.modules}`);
    } catch (e) {
        throw new Error('Error loading MitolServer module');
    }
})();