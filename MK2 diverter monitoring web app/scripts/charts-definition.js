// Credits to: Sara Santos

// Create the charts when the web page loads

window.addEventListener('load', onload);

function onload(event){
  chartP = createPowerChart();
  chartD = createDivertedChart();  
  chartL = createLoadChart();
}

// Create power_at_load Chart
function createLoadChart() {
  var chart = new Highcharts.Chart({
    chart:{ 
      renderTo:'chart-powerAtLoad',
      type: 'spline' 
    },
    series: [
      {
        name: 'POWER AT LOAD'
      }
    ],
    title: { 
      text: undefined
    },
    plotOptions: {
      line: { 
        animation: false,
        dataLabels: { 
          enabled: true 
        }
      }
    },
    xAxis: {
      type: 'datetime',
      dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
      title: { 
        text: 'W' 
      }
    },
    credits: { 
      enabled: false 
    }
  });
  return chart;
}

// Create Humidity Chart
function createDivertedChart(){
  var chart = new Highcharts.Chart({
    chart:{ 
      renderTo:'chart-diverted-energy',
      type: 'spline'  
    },
    series: [{
      name: 'ENERGY DIVERTED'
    }],
    title: { 
      text: undefined
    },    
    plotOptions: {
      line: { 
        animation: false,
        dataLabels: { 
          enabled: true 
        }
      },
      series: { 
        color: '#50b8b4' 
      }
    },
    xAxis: {
      type: 'datetime',
      dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
      title: { 
        text: 'W/h' 
      }
    },
    credits: { 
      enabled: false 
    }
  });
  return chart;
}

// Create Power at supply
function createPowerChart() {
  var chart = new Highcharts.Chart({
    chart:{ 
      renderTo:'chart-power-supply',
      type: 'spline'  
    },
    series: [{
      name: 'POWER SUPPLY'
    }],
    title: { 
      text: undefined
    },    
    plotOptions: {
      line: { 
        animation: false,
        dataLabels: { 
          enabled: true 
        }
      },
      series: { 
        color: '#A62639' 
      }
    },
    xAxis: {
      type: 'datetime',
      dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
      title: { 
        text: 'W' 
      }
    },
    credits: { 
      enabled: false 
    }
  });
  return chart;
}
