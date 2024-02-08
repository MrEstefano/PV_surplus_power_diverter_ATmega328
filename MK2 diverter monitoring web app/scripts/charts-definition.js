// Credits to: Sara Santos

// Create the charts when the web page loads
window.addEventListener('load', onload);

function onload(event){
  chartP = createPowerChart();
  //chartV = createVoltageChart();
  chartD = createDivertedChart();  
  //chartH = createHumidityChart();
  chartT = createTemperatureChart();
}

// Create Temperature Chart
function createTemperatureChart() {
  var chart = new Highcharts.Chart({
    chart:{ 
      renderTo:'chart-temperature',
      type: 'spline' 
    },
    series: [
      {
        name: 'DEVICE TEMP'
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
        text: 'Degree Celsius' 
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
        text: 'Watts' 
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
        text: 'Watts' 
      }
    },
    credits: { 
      enabled: false 
    }
  });
  return chart;
}
