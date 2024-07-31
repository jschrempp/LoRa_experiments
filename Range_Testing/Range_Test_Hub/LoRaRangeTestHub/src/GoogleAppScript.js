function test() {
    var e = {};
    e.parameter = {};
    e.parameter.event = 'sheetTest1';
    e.parameter.data = 'Any Ki{nd & of te,st \r\n data "can ] go: here"';
    e.parameter.coreid = '1f0030001647ffffffffffff';
    e.parameter.published_at = new Date().toISOString();
    doPost(e);
  }
  
  function doGet(e) { 
    var ss = SpreadsheetApp.openByUrl("https://docs.google.com/spreadsheets/d/1SDjUns8Wz5xxxxxxxxx/edit?gid=0#gid=0");
    var sheet = ss.getSheetByName("NewData");
    addLogRow(e,sheet);
  }
  
  function doPost(e) { 
    var ss = SpreadsheetApp.openByUrl("https://docs.google.com/spreadsheets/d/1SDjUns8Wz5xxxxxxxxx/edit?gid=0#gid=0");
    var sheet = ss.getSheetByName("NewData"); 
    addLogRow(e, sheet);
  }
  
  function addLogRow(e, sheet) {
  
    // time from the Google cloud - useful for debugging
    var now = new Date();
    var timePST = Utilities.formatDate(now, "America/Los_Angeles", "yyyy-MM-dd' 'HH:mm:ss");
  
    try{
      // unpack parameters from Particle cloud integration 
      var ev = e.parameter.event;
      var coreid = e.parameter.coreid;
      var timeParticle = e.parameter.published_at;
      var hubData = e.parameter.data;
      
      // add a row to the sheet
      sheet.appendRow([timePST, coreid, timeParticle, hubData] );
  
    } catch(error) {  
      sheet.appendRow(["Error in GApp: " + error, "PostData: " + JSON.stringify(e.postData)]);
    }
  
    // keep the number of rows within bounds by deleting the oldest entries
    cleanUpSheet(sheet);  
  
    return 0;
  }
  
  function cleanUpSheet(sheet) {
    
    const MAX_ROWS = 3000;  // delete some rows if sheet has more than this
    const ROWS_TO_DELETE = 500;  // number of oldest rows to delete in a cleanup operation
    
    var lastRow = sheet.getLastRow();
    
    if(lastRow >= MAX_ROWS) {
      sheet.deleteRows(2, ROWS_TO_DELETE); // keep header row
    }
    
  }
  