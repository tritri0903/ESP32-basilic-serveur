function onRangeChange(rangeInputElmt, listener) {

    var inputEvtHasNeverFired = true;
  
    var rangeValue = {current: undefined, mostRecent: undefined};
    
    rangeInputElmt.addEventListener("input", function(evt) {
      inputEvtHasNeverFired = false;
      rangeValue.current = evt.target.value;
      if (rangeValue.current !== rangeValue.mostRecent) {
        listener(evt);
      }
      rangeValue.mostRecent = rangeValue.current;
    });
  
    rangeInputElmt.addEventListener("change", function(evt) {
      if (inputEvtHasNeverFired) {
        listener(evt);
      }
    });
  }
  
  // example usage:
  
  var myRangeInputElmt = document.getElementById("userSleepTime");
  var myRangeValPar    = document.getElementById("rangeValPar");
  
  var myListener = function(myEvt) {
    t = myEvt.target.value/4;
    if(t <= 23){
    myRangeValPar.innerHTML = "Sleep for: " + myEvt.target.value/4 + " hours";
    }
    else{myRangeValPar.innerHTML = "Sleep until manual wakeup"}
  };

  function updateSliderPWM(element) {
    var sliderValue = document.getElementById("userSleepTime").value;
    console.log(sliderValue);
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/slider?value="+sliderValue, true);
    xhr.send();
  }
  
  onRangeChange(myRangeInputElmt, myListener);