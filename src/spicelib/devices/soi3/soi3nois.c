/**********
STAG version 2.7
Copyright 2000 owned by the United Kingdom Secretary of State for Defence
acting through the Defence Evaluation and Research Agency.
Developed by :     Jim Benson,
                   Department of Electronics and Computer Science,
                   University of Southampton,
                   United Kingdom.
With help from :   Nele D'Halleweyn, Bill Redman-White, and Craig Easson.

Based on STAG version 2.1
Developed by :     Mike Lee,
With help from :   Bernard Tenbroek, Bill Redman-White, Mike Uren, Chris Edwards
                   and John Bunyan.
Acknowledgements : Rupert Howes and Pete Mole.
**********/

/********** 
Modified by Paolo Nenzi 2002
ngspice integration
**********/

#include "ngspice.h"
#include "soi3defs.h"
#include "cktdefs.h"
#include "iferrmsg.h"
#include "noisedef.h"
#include "const.h"
#include "suffix.h"


/* This routine is VERY closely based on the standard MOS noise function.
 * SOI3noise (mode, operation, firstModel, ckt, data, OnDens)
 *    This routine names and evaluates all of the noise sources
 *    associated with MOSFET's.  It starts with the model *firstModel and
 *    traverses all of its insts.  It then proceeds to any other models
 *    on the linked list.  The total output noise density generated by
 *    all of the MOSFET's is summed with the variable "OnDens".
 */


int
SOI3noise (int mode, int operation, GENmodel *genmodel, CKTcircuit *ckt, 
           Ndata *data, double *OnDens)
    
{
    SOI3model *firstModel = (SOI3model *) genmodel;
    SOI3model *model;
    SOI3instance *inst;
    char name[N_MXVLNTH];
    double tempOnoise;
    double tempInoise;
    double noizDens[SOI3NSRCS];
    double lnNdens[SOI3NSRCS];
    double gain;
    double EffectiveLength;
    int i;

    /* define the names of the noise sources */



    static char *SOI3nNames[SOI3NSRCS] = {       /* Note that we have to keep the order */
	"_rd",              /* noise due to rd */        /* consistent with the index definitions */
	"_rs",              /* noise due to rs */        /* in SOI3defs.h */
	"_id",              /* noise due to id */
	"_1overf",          /* flicker (1/f) noise */
	""                  /* total transistor noise */
    };

    for (model=firstModel; model != NULL; model=model->SOI3nextModel) {
	for (inst=model->SOI3instances; inst != NULL; inst=inst->SOI3nextInstance) {
	    
	     if (inst->SOI3owner != ARCHme)
	             continue;
	    
	    switch (operation) {

	    case N_OPEN:

		/* see if we have to to produce a summary report */
		/* if so, name all the noise generators */

		if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
		    switch (mode) {

		    case N_DENS:
			for (i=0; i < SOI3NSRCS; i++) {

			    (void)sprintf(name,"onoise_%s%s",inst->SOI3name,SOI3nNames[i]);

data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
if (!data->namelist) return(E_NOMEM);
		SPfrontEnd->IFnewUid (ckt,
			&(data->namelist[data->numPlots++]),
			(IFuid)NULL, name, UID_OTHER, NULL);
				/* we've added one more plot */


			}
			break;

		    case INT_NOIZ:
			for (i=0; i < SOI3NSRCS; i++) {

			    (void)sprintf(name,"onoise_total_%s%s",inst->SOI3name,SOI3nNames[i]);

data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
if (!data->namelist) return(E_NOMEM);
		SPfrontEnd->IFnewUid (ckt,
			&(data->namelist[data->numPlots++]),
			(IFuid)NULL, name, UID_OTHER, NULL);
				/* we've added one more plot */

             (void)sprintf(name,"inoise_total_%s%s",inst->SOI3name,SOI3nNames[i]);


data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
if (!data->namelist) return(E_NOMEM);
		SPfrontEnd->IFnewUid (ckt,
			&(data->namelist[data->numPlots++]),
			(IFuid)NULL, name, UID_OTHER, NULL);
				/* we've added one more plot */



			}
			break;
		    }
		}
		break;

	    case N_CALC:
		switch (mode) {

		case N_DENS:
/* just get gain from eval routine. Do thermal 
 * noise ourselves as we have local temperature
 * rise.  Also can use channel charge so model
 * is valid in ALL regions and not just saturation.
 */
                    EffectiveLength=inst->SOI3l - 2*model->SOI3latDiff;
		    NevalSrc(&noizDens[SOI3RDNOIZ],(double*)NULL,
				 ckt,N_GAIN,inst->SOI3dNodePrime,inst->SOI3dNode,
				 (double)0.0);
		    noizDens[SOI3RDNOIZ] *= 4 * CONSTboltz *
					    (ckt->CKTtemp + *(ckt->CKTstate0 + inst->SOI3deltaT)) *
					    inst->SOI3drainConductance * inst->SOI3m;
		    lnNdens[SOI3RDNOIZ] = log(MAX(noizDens[SOI3RDNOIZ],N_MINLOG));

		    NevalSrc(&noizDens[SOI3RSNOIZ],(double*)NULL,
				 ckt,N_GAIN,inst->SOI3sNodePrime,inst->SOI3sNode,
				 (double)0.0);
		    noizDens[SOI3RSNOIZ] *= 4 * CONSTboltz *
					    (ckt->CKTtemp + *(ckt->CKTstate0 + inst->SOI3deltaT)) *
					    inst->SOI3sourceConductance * inst->SOI3m;
		    lnNdens[SOI3RSNOIZ] = log(MAX(noizDens[SOI3RSNOIZ],N_MINLOG));

		    NevalSrc(&gain,(double*)NULL,ckt,
				 N_GAIN,inst->SOI3dNodePrime, inst->SOI3sNodePrime,
				 (double)0.0);

		    noizDens[SOI3IDNOIZ] = (gain * 4 * CONSTboltz *
		                            (ckt->CKTtemp + *(ckt->CKTstate0 + inst->SOI3deltaT)) *
		                            inst->SOI3ueff * inst->SOI3m *
		                            fabs(*(ckt->CKTstate0 + inst->SOI3qd) +
		                                 *(ckt->CKTstate0 + inst->SOI3qs)))/
		                           (EffectiveLength*EffectiveLength);
		    lnNdens[SOI3IDNOIZ] = log(MAX(noizDens[SOI3IDNOIZ],N_MINLOG));

                    switch (model->SOI3nLev) {
                    case 2:
                      noizDens[SOI3FLNOIZ] = gain * model->SOI3fNcoef *
                                 (inst->SOI3gmf * inst->SOI3m)*(inst->SOI3gmf * inst->SOI3m)/
                                 (model->SOI3frontOxideCapFactor *
                                  inst->SOI3w * inst->SOI3m * EffectiveLength *
                                  exp(model->SOI3fNexp *
                                      log(MAX(fabs(data->freq),N_MINLOG)))
                                 );
		      break;

                    case 1:
                      noizDens[SOI3FLNOIZ] = gain * model->SOI3fNcoef * 
				 exp(model->SOI3fNexp *
				 log(MAX(fabs(inst->SOI3id * inst->SOI3m),N_MINLOG))) /
				 (data->freq * EffectiveLength * inst->SOI3w * inst->SOI3m *
				  model->SOI3frontOxideCapFactor);
		      break;

                    case 0:
                    default:
		      noizDens[SOI3FLNOIZ] = gain * model->SOI3fNcoef * 
				 exp(model->SOI3fNexp *
				 log(MAX(fabs(inst->SOI3id),N_MINLOG))) /
				 (data->freq * EffectiveLength * EffectiveLength *
				  model->SOI3frontOxideCapFactor);
		      break;
		    }




		    lnNdens[SOI3FLNOIZ] = 
				 log(MAX(noizDens[SOI3FLNOIZ],N_MINLOG));

		    noizDens[SOI3TOTNOIZ] = noizDens[SOI3RDNOIZ] +
						     noizDens[SOI3RSNOIZ] +
						     noizDens[SOI3IDNOIZ] +
						     noizDens[SOI3FLNOIZ];
		    lnNdens[SOI3TOTNOIZ] = 
				 log(MAX(noizDens[SOI3TOTNOIZ], N_MINLOG));

		    *OnDens += noizDens[SOI3TOTNOIZ];

		    if (data->delFreq == 0.0) { 

			/* if we haven't done any previous integration, we need to */
			/* initialize our "history" variables                      */

			for (i=0; i < SOI3NSRCS; i++) {
			    inst->SOI3nVar[LNLSTDENS][i] = lnNdens[i];
			}

			/* clear out our integration variables if it's the first pass */

			if (data->freq == ((NOISEAN*)ckt->CKTcurJob)->NstartFreq) {
			    for (i=0; i < SOI3NSRCS; i++) {
				inst->SOI3nVar[OUTNOIZ][i] = 0.0;
				inst->SOI3nVar[INNOIZ][i] = 0.0;
			    }
			}
		    } else {   /* data->delFreq != 0.0 (we have to integrate) */
			for (i=0; i < SOI3NSRCS; i++) {
			    if (i != SOI3TOTNOIZ) {
				tempOnoise = Nintegrate(noizDens[i], lnNdens[i],
				      inst->SOI3nVar[LNLSTDENS][i], data);
				tempInoise = Nintegrate(noizDens[i] * data->GainSqInv ,
				      lnNdens[i] + data->lnGainInv,
				      inst->SOI3nVar[LNLSTDENS][i] + data->lnGainInv,
				      data);
				inst->SOI3nVar[LNLSTDENS][i] = lnNdens[i];
				data->outNoiz += tempOnoise;
				data->inNoise += tempInoise;
				if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
				    inst->SOI3nVar[OUTNOIZ][i] += tempOnoise;
				    inst->SOI3nVar[OUTNOIZ][SOI3TOTNOIZ] += tempOnoise;
				    inst->SOI3nVar[INNOIZ][i] += tempInoise;
				    inst->SOI3nVar[INNOIZ][SOI3TOTNOIZ] += tempInoise;
                                }
			    }
			}
		    }
		    if (data->prtSummary) {
			for (i=0; i < SOI3NSRCS; i++) {     /* print a summary report */
			    data->outpVector[data->outNumber++] = noizDens[i];
			}
		    }
		    break;

		case INT_NOIZ:        /* already calculated, just output */
		    if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
			for (i=0; i < SOI3NSRCS; i++) {
			    data->outpVector[data->outNumber++] = inst->SOI3nVar[OUTNOIZ][i];
			    data->outpVector[data->outNumber++] = inst->SOI3nVar[INNOIZ][i];
			}
		    }    /* if */
		    break;
		}    /* switch (mode) */
		break;

	    case N_CLOSE:
		return (OK);         /* do nothing, the main calling routine will close */
		break;               /* the plots */
	    }    /* switch (operation) */
	}    /* for inst */
    }    /* for model */

return(OK);
}
